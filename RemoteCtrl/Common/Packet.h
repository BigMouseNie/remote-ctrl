#pragma once

#define PACKETHEADERMAGIC	0xFEFF
#define PACKETVERSION		0x0001	// version : 0.0.0.1
#define EXPANSIONFACTOR		2
#define RECVMINBUFSIZE		2048
#define EXTRACTIONCMD		0x0FFFFFFF

#include <mutex>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

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

enum ChecksumType : uint16_t
{
	CT_NONE,
	CT_SUM,
	CT_CRC16,
	CT_CRC32,
	CT_INVALID_VALUE,
};

enum StatusCode : uint32_t
{
	SC_NONE			= 0x00000000,
	SC_OK			= 0x00001000,
	SC_OK_WAIT 		= 0x00001001,
	SC_ERR			= 0x10000000,
	SC_ERR_INTERNAL = 0x10010000,
	SC_ERR_PACKET	= 0x10020000,
	SC_ERR_NOTFOUND = 0x10030000,
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

struct PacketHeader
{
	uint16_t magic;					// 固定魔数 0xFEFF，用于包起始验证
	uint16_t version;				// 协议版本号
	uint16_t checksumType;			// 校验方式
	uint32_t checksum;				// 校验值
	uint32_t statusCode;			// 状态
	uint32_t handleID;
	MessageType messageType;		// 消息类型
	uint32_t bodyLength;			// 包体长度

	PacketHeader()
		: magic(0), version(0), checksumType(0), checksum(0),
		statusCode(0), handleID(0), messageType(0), bodyLength(0)
	{}
	~PacketHeader() = default;

	void Clear() {
		magic = 0;
		version = 0;
		checksumType = 0;
		checksum = 0;
		statusCode = 0;
		handleID = 0;
		messageType = 0;
		bodyLength = 0;
	}
	static bool Serialize(const PacketHeader& inHeader, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, PacketHeader& outHeader);
	static const size_t packetHeaderCompactSize;
};

struct Packet
{
	PacketHeader header;
	Buffer body;
	void Clear() {
		body.Clear();
		header.Clear();
	}

	Packet& operator=(const Packet& other) {
		body.Clear();
		other.body.TryPeekExact(body, other.body.Readable());
		header = other.header;
		return *this;
	}
};

enum class RecvState : size_t
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
	int SendPacket(const PacketHeader* sHeader, const char* body);
	int SendPacket(const PacketHeader* sHeader, const Buffer& body);
	int SendPacket(const Packet& packet);
	int SafeSendPacket(const Packet& packet);

	void SetSocket(SOCKET sock, bool isResetBuf = true);

	static bool ValidatePacket(const Packet& packet, MessageType msgType, bool isValidateMsgType = true);
	static void BuildPacketHeader(Packet& packet, StatusCode scCode,
								  ChecksumType csType, MessageType msgType);
	static void BuildPacket(Packet& packet, const char* body, size_t bodySize,
							StatusCode scCode, ChecksumType csType, MessageType msgType);

	static void BuildPacketHeaderInPacket(Packet& outPacket, StatusCode scCode,
		ChecksumType csType, uint32_t handleID, MessageType msgType);
	static void BuildPacket(Packet& outPacket, const char* body, size_t bodyLen,
		StatusCode scCode, ChecksumType csType, uint32_t handleID, MessageType msgType);

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
