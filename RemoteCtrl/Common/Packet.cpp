#include "pch.h"
#include "Packet.h"

const size_t PacketHeader::packetHeaderCompactSize =
		sizeof(uint16_t) + // magic
		sizeof(uint16_t) + // version
		sizeof(uint16_t) + // checksumType
		sizeof(uint32_t) + // checksum
		sizeof(uint32_t) + // statusCode
		sizeof(uint32_t) + // handleID
		sizeof(MessageType) + // messageType
		sizeof(uint32_t);  // bodyLength

Buffer::Buffer()
	: data(nullptr), readptr(nullptr), writeptr(nullptr), readable(0), writable(0), size(0)
{}

Buffer::Buffer(size_t bufSize)
{
	data = new char[bufSize];
	readptr = data;
	writeptr = data;
	readable = 0;
	writable = bufSize;
	size = bufSize;
}

Buffer::~Buffer()
{
	if (data) { delete[] data; }
	data = nullptr;
	readptr = nullptr;
	writeptr = nullptr;
}

void Buffer::Clear()
{
	readptr = data;
	writeptr = data;
	readable = 0;
	writable = size;
}

bool Buffer::Write(const char* src, size_t lenToWrite)
{
	EnsureWritableSize(lenToWrite);
	memcpy(writeptr, src, lenToWrite);
	Written(lenToWrite);
	return true;
}

bool Buffer::Write(const Buffer& srcPeekBuf, size_t lenToWrite)
{
	EnsureWritableSize(lenToWrite);

	memcpy(writeptr, srcPeekBuf.readptr, lenToWrite);
	Written(lenToWrite);
	return true;
}

bool Buffer::Write(Buffer& srcBuf, size_t lenToWrite)
{
	if (Write(const_cast<const Buffer&>(srcBuf), lenToWrite)) {
		srcBuf.ReadOut(lenToWrite);
		return true;
	}

	return false;
}

bool Buffer::TryPeekExact(char* dest, size_t lenToRead) const
{
	if (readable < lenToRead) {
		return false;
	}
	memcpy(dest, readptr, lenToRead);
	return true;
}

bool Buffer::TryReadExact(char* dest, size_t lenToRead)
{
	if (TryPeekExact(dest, lenToRead)) {
		ReadOut(lenToRead);
		return true;
	}

	return false;
}

bool Buffer::TryPeekExact(Buffer& destBuf, size_t lenToRead) const
{
	if (readable < lenToRead) {
		return false;
	}

	destBuf.EnsureWritableSize(lenToRead);
	memcpy(destBuf.writeptr, readptr, lenToRead);
	destBuf.Written(lenToRead);
	return true;
}

bool Buffer::TryReadExact(Buffer& destBuf, size_t lenToRead)
{
	if (TryPeekExact(destBuf, lenToRead)) {
		ReadOut(lenToRead);
		return true;
	}

	return false;
}

/**
 * @return : 返回接收的字节数(不会返回0)，返回-1需要关闭连接
 */
int Buffer::Recv(SOCKET sock)
{
	EnsureWritableSize(RECVMINBUFSIZE);
	int len = recv(sock, writeptr, writable, 0);
	if (len > 0) {
		Written(len);
		return len;
	}
	return -1;
}

/**
 * @return : 返回发送的字节数(不会返回0)，返回-1需要关闭连接
 */
int Buffer::Send(SOCKET sock)
{
	size_t total = 0;
	while (total < readable) {
		int len = send(sock, readptr, readable, 0);
		if (len > 0) {
			ReadOut(len);
			total += len;
		}
		else {
			return -1;
		}
	}
	return total;
}

size_t Buffer::Readable() const
{
	return readable;
}

size_t Buffer::Writable() const
{
	return writable;
}

size_t Buffer::CompactWritable() const
{
	return size - readable;
}

const char* Buffer::GetReadPtr() const
{
	return readptr;
}

const char* Buffer::GetWritePtr() const
{
	return writeptr;
}

void Buffer::Written(size_t bytes)
{
	readable += bytes;
	writable -= bytes;
	writeptr += bytes;
}

void Buffer::ReadOut(size_t bytes)
{
	readable -= bytes;
	readptr += bytes;
}

void Buffer::Compact()
{
	memcpy(data, readptr, readable);
	readptr = data;
	writeptr = readptr + readable;
	writable = size - readable;
}

void Buffer::Expand(size_t newCap)
{
	char* newData = new char[newCap];
	memcpy(newData, readptr, readable);
	readptr = newData;
	writeptr = readptr + readable;
	writable = newCap - readable;
	size = newCap;
	delete[] data;
	data = newData;
}

void Buffer::EnsureWritableSize(size_t n)
{
	if (writable < n) {
		if (size - readable < n) {
			Expand(readable + n * EXPANSIONFACTOR);
		}
		else {
			Compact();
		}
	}
}

bool PacketHeader::Serialize(const PacketHeader& inHeader, Buffer& outStream)
{
	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.magic)), sizeof(inHeader.magic))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.version)), sizeof(inHeader.version))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.checksumType)), sizeof(inHeader.checksumType))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.checksum)), sizeof(inHeader.checksum))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.statusCode)), sizeof(inHeader.statusCode))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.handleID)), sizeof(inHeader.handleID))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.messageType)), sizeof(inHeader.messageType))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inHeader.bodyLength)), sizeof(inHeader.bodyLength))) {
		outStream.Clear(); return false;
	}

	return true;
}

bool PacketHeader::Deserialize(const Buffer& inStream, PacketHeader& outHeader)
{
	const char* inReadPtr = inStream.GetReadPtr();
	size_t readable = inStream.Readable();

	if (readable < packetHeaderCompactSize) {
		return false;
	}

	const uint16_t* u16Arr = reinterpret_cast<const uint16_t*>(inReadPtr);
	outHeader.magic = u16Arr[0];
	outHeader.version = u16Arr[1];
	outHeader.checksumType = u16Arr[2];

	const uint32_t* u32Arr = reinterpret_cast<const uint32_t*>(inReadPtr + (sizeof(uint16_t) * 3));
	outHeader.checksum = u32Arr[0];
	outHeader.statusCode = u32Arr[1];
	outHeader.handleID = u32Arr[2];
	outHeader.messageType = u32Arr[3];
	outHeader.bodyLength = u32Arr[4];

	return true;
}

CPacketHandler::CPacketHandler()
	: sock(INVALID_SOCKET), state(RecvState::WAIT_HEADER)
{}

CPacketHandler::CPacketHandler(SOCKET sock)
	: sock(sock), state(RecvState::WAIT_HEADER)
{}

CPacketHandler::~CPacketHandler()
{
	sock = INVALID_SOCKET;	// 需要上层应用关闭
}

void CPacketHandler::SetSocket(SOCKET sock, bool isResetBuf)
{
	if (isResetBuf) {
		recvbuf.Clear();
		sendbuf.Clear();
		forRecvPHBuf.Clear();
	}
	this->sock = sock;
}

int CPacketHandler::GetPacket(Packet& packet)
{
	int res = ParseBuffer(packet);
	if (res != 0) { return res; }

	int readLen = recvbuf.Recv(sock);
	if (readLen <= 0) {
		return -1;
	}

	return ParseBuffer(packet);
}

int CPacketHandler::ParseBuffer(Packet& packet)
{
	if (state == RecvState::WAIT_HEADER && recvbuf.TryReadExact(forRecvPHBuf, PacketHeader::packetHeaderCompactSize)) {
		bool res = PacketHeader::Deserialize(forRecvPHBuf, curRecvPH);
		forRecvPHBuf.Clear();
		if (!res || curRecvPH.magic != PACKETHEADERMAGIC) {
			return -1;
		}
		state = RecvState::WAIT_BODY;
		recvbuf.EnsureWritableSize(curRecvPH.bodyLength);
	}

	if (state == RecvState::WAIT_BODY && recvbuf.TryReadExact(packet.body, curRecvPH.bodyLength)) {
		packet.header = curRecvPH;
		state = RecvState::WAIT_HEADER;
		return curRecvPH.bodyLength + sizeof(PacketHeader);
	}

	return 0;
}

int CPacketHandler::SendPacket(const PacketHeader* sHeader, const char* body)
{
	sendbuf.Clear();
	PacketHeader::Serialize(*sHeader, sendbuf);
	sendbuf.Write(body, sHeader->bodyLength);
	return sendbuf.Send(sock);
}

int CPacketHandler::SendPacket(const PacketHeader* sHeader, const Buffer& body)
{
	sendbuf.Clear();
	PacketHeader::Serialize(*sHeader, sendbuf);
	sendbuf.Write(body, sHeader->bodyLength);
	return sendbuf.Send(sock);
}

int CPacketHandler::SendPacket(const Packet& packet)
{
	return SendPacket(&(packet.header), packet.body);
}

int CPacketHandler::SafeSendPacket(const Packet& packet)
{
	std::lock_guard<std::mutex> sendLock(sendMtx);
	return SendPacket(packet);
}

bool CPacketHandler::ValidatePacket(const Packet& packet, MessageType msgType, bool isValidateMsgType)
{
	const PacketHeader& ph = packet.header;
	if (ph.magic != PACKETHEADERMAGIC ||
		ph.version != PACKETVERSION)
	{
		return false;
	}

	if (isValidateMsgType && ph.messageType != msgType) {
		return false;
	}

	if (ph.bodyLength != packet.body.Readable()) {
		return false;
	}

	// TODO: 校验和
	const char* pdata = packet.body.GetReadPtr();
	if (ph.checksum != CalculateChecksum(static_cast<ChecksumType>(ph.checksumType),
		reinterpret_cast<const uint8_t*>(pdata), ph.bodyLength))
	{
		return false;
	}

	return true;
}

void CPacketHandler::BuildPacketHeader(Packet& packet, StatusCode scCode,
	ChecksumType csType, MessageType msgType)
{
	PacketHeader& ph = packet.header;
	ph.magic = PACKETHEADERMAGIC;
	ph.version = PACKETVERSION;
	ph.statusCode = scCode;
	ph.checksumType = csType;
	ph.checksum = CalculateChecksum(csType, reinterpret_cast<const uint8_t*>(packet.body.GetReadPtr()),
		packet.body.Readable());
	ph.messageType = msgType;
	ph.bodyLength = packet.body.Readable();
}

void CPacketHandler::BuildPacket(Packet& packet, const char* body, size_t bodySize,
	StatusCode scCode, ChecksumType csType, MessageType msgType)
{
	packet.body.Clear();
	packet.body.Write(body, bodySize);
	BuildPacketHeader(packet, scCode, csType, msgType);
}

void CPacketHandler::BuildPacketHeaderInPacket(Packet& outPacket, StatusCode scCode,
	ChecksumType csType, uint32_t handleID, MessageType msgType)
{
	PacketHeader& ph = outPacket.header;
	ph.magic = PACKETHEADERMAGIC;
	ph.version = PACKETVERSION;
	ph.statusCode = scCode;
	ph.checksumType = csType;
	ph.checksum = CalculateChecksum(csType, reinterpret_cast<const uint8_t*>(outPacket.body.GetReadPtr()),
		outPacket.body.Readable());
	ph.handleID = handleID;
	ph.messageType = msgType;
	ph.bodyLength = outPacket.body.Readable();
}

void CPacketHandler::BuildPacket(Packet& outPacket, const char* body, size_t bodyLen,
	StatusCode scCode, ChecksumType csType, uint32_t handleID, MessageType msgType)
{
	outPacket.body.Write(body, bodyLen);
	BuildPacketHeaderInPacket(outPacket, scCode, csType, handleID, msgType);
}

uint32_t CPacketHandler::CalculateChecksum(ChecksumType type, const uint8_t* data, size_t len)
{
	switch (type) {
	case ChecksumType::CT_SUM:
		return CalculateSum(data, len);
	case ChecksumType::CT_CRC16:
		return CalculateCRC16(data, len);
	case ChecksumType::CT_CRC32:
		return CalculateCRC32(data, len);
	case ChecksumType::CT_NONE:
	default:
		return 0;
	}
}

uint16_t CPacketHandler::CalculateSum(const uint8_t* data, size_t len)
{
	uint16_t sum = 0;
	for (size_t i = 0; i < len; ++i) {
		sum += data[i];
	}
	return sum;
}

uint16_t CPacketHandler::CalculateCRC16(const uint8_t* data, size_t len)
{
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; ++i) {
		crc ^= static_cast<uint16_t>(data[i]) << 8;
		for (int j = 0; j < 8; ++j) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}

uint32_t CPacketHandler::CalculateCRC32(const uint8_t* data, size_t len)
{
	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < len; ++i) {
		crc ^= data[i];
		for (int j = 0; j < 8; ++j) {
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}
	return ~crc;
}
