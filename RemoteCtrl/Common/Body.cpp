#include "pch.h"
#include "Body.h"

bool FileListBody::Serialize(const FileListBody& inFLBody, Buffer& outStream)
{
	if (!outStream.Write(reinterpret_cast<const char*>(&(inFLBody.fileCnt)), sizeof(inFLBody.fileCnt))) {
		outStream.Clear(); return false;
	}
	if (!outStream.Write(inFLBody.fileTypes, inFLBody.fileTypes.Readable())) {
		outStream.Clear(); return false;
	}
	if (!outStream.Write(inFLBody.fileNameLens, inFLBody.fileNameLens.Readable())) {
		outStream.Clear(); return false;
	}
	if (!outStream.Write(inFLBody.fileNameBuffer, inFLBody.fileNameBuffer.Readable())) {
		outStream.Clear(); return false;
	}

	return true;
}

bool FileListBody::Deserialize(const Buffer& inStream, FileListBody& outFLBody)
{
	const char* inReadPtr = inStream.GetReadPtr();
	size_t readable = inStream.Readable();
	size_t readLen = 0;

	// 填充 fileCnt
	if (readable < sizeof(FileCnt)) {
		outFLBody.Clear(); return false;
	}
	const FileCnt* forFileCnt = reinterpret_cast<const FileCnt*>(inReadPtr);
	outFLBody.fileCnt = forFileCnt[0];
	readable -= sizeof(FileCnt);
	readLen += sizeof(FileCnt);

	// 提前检查长度问题
	size_t fileTypeArrBytes = sizeof(FileType) * outFLBody.fileCnt;
	size_t fileNameArrBytes = sizeof(FileNameLen) * outFLBody.fileCnt;
	if (readable < fileTypeArrBytes + fileNameArrBytes) {
		outFLBody.Clear(); return false;
	}

	// 填充 fileTypes
	if (!outFLBody.fileTypes.Write(inReadPtr + readLen, fileTypeArrBytes)) {
		outFLBody.Clear(); return false;
	}
	readable -= fileTypeArrBytes;
	readLen += fileTypeArrBytes;

	// 填充 fileNameLens
	if (!outFLBody.fileNameLens.Write(inReadPtr + readLen, fileNameArrBytes)) {
		outFLBody.Clear(); return false;
	}
	readable -= fileNameArrBytes;
	readLen += fileNameArrBytes;

	// 填充 fileNameBuffer
	const FileNameLen* fileNameLenArr =
		reinterpret_cast<const FileNameLen*>(outFLBody.fileNameLens.GetReadPtr());
	size_t totalFileNameLen = 0;
	for (FileCnt i = 0; i < outFLBody.fileCnt; ++i) {
		totalFileNameLen += fileNameLenArr[i];
	}
	if (readable < totalFileNameLen) {
		outFLBody.Clear(); return false;
	}
	if (!outFLBody.fileNameBuffer.Write(inReadPtr + readLen, totalFileNameLen)) {
		outFLBody.Clear(); return false;
	}

	return true;
}

const size_t ReqFileBody::minCompactSize =
		sizeof(uint32_t) + // fileID
		sizeof(uint32_t) + // ctrlCode
		sizeof(uint32_t); // filePathLen
bool ReqFileBody::Serialize(const ReqFileBody& inRFBody, Buffer& outStream)
{
	if (!outStream.Write(reinterpret_cast<const char*>(&(inRFBody.fileID)), sizeof(inRFBody.fileID))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inRFBody.ctrlCode)), sizeof(inRFBody.ctrlCode))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inRFBody.filePathLen)), sizeof(inRFBody.filePathLen))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(inRFBody.filePath, inRFBody.filePathLen)) {
		outStream.Clear(); return false;
	}

	return true;
}

bool ReqFileBody::Deserialize(const Buffer& inStream, ReqFileBody& outRFBody)
{
	const char* inReadPtr = inStream.GetReadPtr();
	size_t readable = inStream.Readable();

	if (readable < ReqFileBody::minCompactSize) {
		return false;
	}

	const uint32_t* u32Arr = reinterpret_cast<const uint32_t*>(inReadPtr);
	outRFBody.fileID = u32Arr[0];
	outRFBody.ctrlCode = u32Arr[1];
	outRFBody.filePathLen = u32Arr[2];

	inReadPtr += (sizeof(uint32_t) * 3);
	outRFBody.filePath.Write(inReadPtr, outRFBody.filePathLen);

	return true;
}

const size_t FileBody::minCompactSize =
		sizeof(uint32_t) + // fileID
		sizeof(uint32_t) + // isLastChunk
		sizeof(uint32_t) + // chunkSize
		sizeof(uint64_t) + // offset
		sizeof(uint64_t);  // totalSize

void FileBody::SerializeNoSafe(const FileBody& inFBody, char* outStream)
{
	char* outWritePtr = outStream;
	memcpy(outWritePtr, reinterpret_cast<const char*>(&(inFBody.fileID)), sizeof(inFBody.fileID));
	outWritePtr += sizeof(inFBody.fileID);

	memcpy(outWritePtr, reinterpret_cast<const char*>(&(inFBody.isLastChunk)), sizeof(inFBody.isLastChunk));
	outWritePtr += sizeof(inFBody.isLastChunk);

	memcpy(outWritePtr, reinterpret_cast<const char*>(&(inFBody.chunkSize)), sizeof(inFBody.chunkSize));
	outWritePtr += sizeof(inFBody.chunkSize);

	memcpy(outWritePtr, reinterpret_cast<const char*>(&(inFBody.offset)), sizeof(inFBody.offset));
	outWritePtr += sizeof(inFBody.offset);

	memcpy(outWritePtr, reinterpret_cast<const char*>(&(inFBody.totalSize)), sizeof(inFBody.totalSize));
	outWritePtr += sizeof(inFBody.totalSize);

	inFBody.chunk.TryPeekExact(outWritePtr, inFBody.chunk.Readable());
}

bool FileBody::Serialize(const FileBody& inFBody, Buffer& outStream)
{
	if (!outStream.Write(reinterpret_cast<const char*>(&(inFBody.fileID)), sizeof(inFBody.fileID))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inFBody.isLastChunk)), sizeof(inFBody.isLastChunk))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inFBody.chunkSize)), sizeof(inFBody.chunkSize))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inFBody.offset)), sizeof(inFBody.offset))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inFBody.totalSize)), sizeof(inFBody.totalSize))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(inFBody.chunk, inFBody.chunkSize)) {
		outStream.Clear(); return false;
	}

	return true;
}

bool FileBody::Deserialize(const Buffer& inStream, FileBody& outFBody)
{
	const char* inReadPtr = inStream.GetReadPtr();
	size_t readable = inStream.Readable();

	if (readable < FileBody::minCompactSize) {
		return false;
	}

	const uint32_t* u32Arr = reinterpret_cast<const uint32_t*>(inReadPtr);
	outFBody.fileID = u32Arr[0];
	outFBody.isLastChunk = u32Arr[1];
	outFBody.chunkSize = u32Arr[2];
	inReadPtr += (sizeof(uint32_t) * 3);
	readable -= (sizeof(uint32_t) * 3);

	const uint64_t* u64Arr = reinterpret_cast<const uint64_t*>(inReadPtr);
	outFBody.offset = u64Arr[0];
	outFBody.totalSize = u64Arr[1];
	inReadPtr += (sizeof(uint64_t) * 2);
	readable -= (sizeof(uint64_t) * 2);

	if (readable < outFBody.chunkSize) {
		outFBody.Clear(); return false;
	}

	if (!outFBody.chunk.Write(inReadPtr, outFBody.chunkSize)) {
		outFBody.Clear();  return false;
	}

	return true;
}

WHXY MouseEventBody::GetCurWinXY(const WHXY& inCurWinWH) const
{
	return GetCurWinXY(inCurWinWH.WX, inCurWinWH.HY);
}

WHXY MouseEventBody::GetCurWinXY(int curWinW, int curWinH) const
{
	WHXY curWinXY;
	curWinXY.WX = curWinW * relativeXY.WX / senderWinWH.WX;
	curWinXY.HY = curWinH * relativeXY.HY / senderWinWH.HY;
	return curWinXY;
}

bool MouseEventBody::Serialize(const MouseEventBody& inMEBody, Buffer& outStream)
{
	if (!outStream.Write(reinterpret_cast<const char*>(&(inMEBody.senderWinWH)), sizeof(inMEBody.senderWinWH))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inMEBody.relativeXY)), sizeof(inMEBody.relativeXY))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inMEBody.mb)), sizeof(inMEBody.mb))) {
		outStream.Clear(); return false;
	}

	if (!outStream.Write(reinterpret_cast<const char*>(&(inMEBody.mba)), sizeof(inMEBody.mba))) {
		outStream.Clear(); return false;
	}

	return true;
}

bool MouseEventBody::Deserialize(const Buffer& inStream, MouseEventBody& outMEBody)
{
	if (inStream.Readable() < sizeof(outMEBody)) {
		return false;
	}
	const char* inReadPtr = inStream.GetReadPtr();

	const WHXY* whxys = reinterpret_cast<const WHXY*>(inReadPtr);
	outMEBody.senderWinWH = whxys[0];
	outMEBody.relativeXY = whxys[1];
	inReadPtr += (2 * sizeof(WHXY));

	const uint16_t* mbs = reinterpret_cast<const uint16_t*>(inReadPtr);
	outMEBody.mb = static_cast<MouseBtn>(mbs[0]);
	outMEBody.mba = static_cast<MouseBtnAct>(mbs[1]);

	return true;
}
