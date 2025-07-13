#pragma once

#define PACKETHEADERMAGIC	0xFEFF
#define PACKETVERSION		0x01	// version : 1
#define EXPANSIONFACTOR		2
#define RECVMINBUFSIZE		2048
#define EXTRACTIONCMD		0x00FFFFFF

#include <mutex>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

using PacketVersion = uint8_t;
using MessageType = uint32_t;
// MessageType = ReqRes | CommandType
enum ReqRes : MessageType
{
	RR_REQUEST = 0x50000000,
	RR_RESPONSE = 0xA0000000,
};

enum CommandType : MessageType
{
	CMD_TEST,
	CMD_DISK_PART,
	CMD_LIST_FILE,
	CMD_RUN_FILE,
	CMD_DEL_FILE,
	CMD_DOWNLOAD_FILE,
	CMD_SEND_SCREEN,
	CMD_MOUSE_EVENT,
	CMD_LOCK_MACHINE,
	CMD_UNLOCK_MACHINE,
	CMD_INVALID_VALUE,
};

using ChecksumVal = uint32_t;
enum ChecksumType : uint8_t
{
	CT_NONE,
	CT_SUM,
	CT_CRC16,
	CT_CRC32,
	CT_INVALID_VALUE,
};

enum StatusCode : uint16_t
{
	SC_NONE			= 0x0000,
	SC_OK			= 0x0080,
	SC_OK_WAIT 		= 0x0081,
	SC_ERR			= 0x8000,
	SC_ERR_INTERNAL = 0x8100,
	SC_ERR_PACKET	= 0x8200,
	SC_ERR_NOTFOUND = 0x8300,
};

class RawBuffer;
class CPacketHandler;
class Buffer
{
	friend class RawBuffer;
	friend class CPacketHandler;
public:
	explicit Buffer();
	explicit Buffer(size_t bufSize);
	~Buffer();

	void Clear();

	bool Write(const char* src, size_t lenToWrite);

	bool Write(const Buffer& srcPeekBuf, size_t lenToWrite);	// 不移动 srcBuf的读写指针
	bool Write(Buffer& srcBuf, size_t lenToWrite);

	bool TryPeekExact(char* dest, size_t lenToRead) const;
	bool TryReadExact(char* dest, size_t lenToRead);

	bool TryPeekExact(Buffer& destBuf, size_t lenToRead) const;
	bool TryReadExact(Buffer& destBuf, size_t lenToRead);

	int Recv(SOCKET sock);
	int Send(SOCKET sock);
	size_t Readable() const;
	size_t Writable() const;
	size_t CompactWritable() const;

	const char* GetReadPtr() const;
	const char* GetWritePtr() const;

private:
	char* data;						// data
	char* readptr;					// 读指针
	char* writeptr;					// 写指针
	size_t readable;				// 可读字节
	size_t writable;				// 可写字节
	size_t size;					// size

	void Written(size_t bytes);
	void ReadOut(size_t bytes);
	void Compact();
	void Expand(size_t newCap);
	void EnsureWritableSize(size_t n);
};

class RawBuffer
{
public:
	RawBuffer(Buffer& buf, size_t ensureWritableSize)
		: rawBuf(buf)
	{
		rawBuf.EnsureWritableSize(ensureWritableSize);
	}
	~RawBuffer() = default;

	char* GetRawWritePtr() {
		return rawBuf.writeptr;
	}

	void Written(size_t written)
	{
		rawBuf.Written(written);
	}

private:
	Buffer& rawBuf;
};

using BodyLength = uint32_t;
using HandleID = uint32_t;
#pragma pack(push, 1)
struct PacketHeader
{
	uint16_t        magic;					// 固定魔数 0xFEFF，用于包起始验证
    PacketVersion   version;				// 协议版本号
    ChecksumType    checksumType;			// 校验方式
    ChecksumVal     checksumVal;			// 校验值

	MessageType     messageType;		    // 消息类型
    BodyLength      bodyLength;			    // 包体长度

    HandleID        handleID;               // 客户端相关应用程序信息
    StatusCode      statusCode;			    // 状态码

	PacketHeader()
		: magic(0), version(0), checksumType(ChecksumType::CT_NONE), checksumVal(0),
		  messageType(0), statusCode(StatusCode::SC_NONE),bodyLength(0), handleID(0)
	{}
	~PacketHeader() = default;

	void Clear() {
        memset(this, 0, sizeof(PacketHeader));
	}

    PacketHeader& operator=(const PacketHeader& other) {
        if (this != &other) {
            memcpy(reinterpret_cast<char*>(this), reinterpret_cast<const char*>(&other), sizeof(PacketHeader));
        }
        return *this;
    }

	static bool Serialize(const PacketHeader& inHeader, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, PacketHeader& outHeader);
};
#pragma pack(pop)

struct Packet
{
	PacketHeader header;
	Buffer body;
	void Clear() {
		body.Clear();
		header.Clear();
	}

    bool Empty() {
        return body.Readable() == 0 && header.handleID == 0;
    }

	Packet& operator=(const Packet& other) {
        if (this != &other) {
            body.Clear();
            other.body.TryPeekExact(body, other.body.Readable());
            header = other.header;
        }
		return *this;
	}
};

enum RecvState
{
	WAIT_HEADER,					// 等待读包头	
	WAIT_BODY,						// 等待读包体
};

class CPacketHandler
{
public:
	CPacketHandler();
	CPacketHandler(SOCKET sock);
	~CPacketHandler();

	int GetPacket(Packet& packet);	// block
	int SendPacket(const PacketHeader& sHeader, const char* body);
	int SendPacket(const PacketHeader& sHeader, const Buffer& body);
	int SendPacket(const Packet& packet);
	int SafeSendPacket(const Packet& packet);

	void SetSocket(SOCKET sock, bool isResetBuf = true);

	static bool ValidatePacket(const Packet& packet, MessageType msgType, bool isValidateMsgType = true);

    static void BuildPacketHeaderInPacket(Packet& outPacket,
        ChecksumType csType, MessageType msgType, StatusCode scCode);
	static void BuildPacket(Packet& outPacket, const char* body, size_t bodyLen,
        ChecksumType csType, MessageType msgType, StatusCode scCode);

private:
	int ParseBuffer(Packet& packet);

	static uint32_t CalculateChecksum(ChecksumType type, const uint8_t* data, size_t len);
	static uint16_t CalculateSum(const uint8_t* data, size_t len);
	static uint16_t CalculateCRC16(const uint8_t* data, size_t len);
	static uint32_t CalculateCRC32(const uint8_t* data, size_t len);

private:
	SOCKET sock;
	Buffer recvbuf;
	Buffer sendbuf;
	RecvState state;				// 状态
	Buffer forRecvPHBuf;
	PacketHeader curRecvPH;			// state为WAIT_BODY时有效
	std::mutex sendMtx;
};
