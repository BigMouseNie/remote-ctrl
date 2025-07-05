#pragma once

#include "Packet.h"

using FileCnt = uint32_t;
using FileNameLen = uint16_t;

enum FileType : char
{
	FT_FILE,
	FT_DIR,
	FT_INVALID_VALUE,
};

struct FileListBody
{
	FileCnt fileCnt;
	Buffer fileTypes;			// ��FileType(ö��)��ȡ
	Buffer fileNameLens;		// ��FileNameLen��ȡ��ÿ���ļ����ĳ���
	Buffer fileNameBuffer;		// ��char��ȡ��ƴ�Ӻ���ļ���

	FileListBody() : fileCnt(0) {}
	void Clear() {
		fileCnt = 0; fileTypes.Clear(); fileNameLens.Clear(); fileNameBuffer.Clear();
	}
	static bool Serialize(const FileListBody& inFLBody, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, FileListBody& outFLBody);
};

struct ReqFileBody
{
	enum CtrlCode : uint32_t
	{
		RFB_CC_BEGIN,
		RFB_CC_STOP,
		RFB_CC_NEXT,
		RFB_CC_PREV,
		RFB_CC_INVALID_VALUE,
	};
	uint32_t fileID;
	uint32_t ctrlCode;
	uint32_t filePathLen;
	Buffer filePath;
	ReqFileBody() : fileID(0), ctrlCode(RFB_CC_INVALID_VALUE), filePathLen(0) {}
	void Clear() {
		fileID = 0;
		ctrlCode = RFB_CC_INVALID_VALUE;
		filePathLen = 0;
		filePath.Clear();
	}
	static bool Serialize(const ReqFileBody& inRFBody, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, ReqFileBody& outRFBody);
	static const size_t minCompactSize;
};

struct FileBody
{
	uint32_t fileID;			// �ļ�ΨһID����Hashֵ
	uint32_t isLastChunk;		// �Ƿ������һ�飨1 ��ʾ�ǣ�0 ��ʾ���У�
	uint32_t chunkSize;			// ��ǰ������ݳ���
	uint64_t offset;			// ��ǰ���������ļ��е�ƫ��λ��
	uint64_t totalSize;			// �ļ��ܳ���
	Buffer chunk;				// ��ǰ������
	FileBody() :fileID(0), isLastChunk(0), chunkSize(0), offset(0), totalSize(0) {}
	void Clear() {
		fileID = 0;
		isLastChunk = 0;
		chunkSize = 0;
		offset = 0;
		totalSize = 0;
		chunk.Clear();
	}
	static void SerializeNoSafe(const FileBody& inFBody, char* outStream);
	static bool Serialize(const FileBody& inFBody, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, FileBody& outFBody);
	static const size_t minCompactSize;
};

enum MouseBtn : uint16_t
{
	MB_NONE,
	MB_LEFTBTN,
	MB_RIGHTBTN,
	MB_MIDBTN,
};

enum MouseBtnAct : uint16_t
{
	MBA_NONE,
	MBA_DOUBLE,
	MBA_DOWN,
	MBA_UP,
};

struct WHXY
{
	int WX;
	int HY;
};

struct MouseEventBody
{
	// Mouse Location
	WHXY senderWinWH;
	WHXY relativeXY;

	MouseBtn mb;
	MouseBtnAct mba;

	WHXY GetCurWinXY(const WHXY& inCurWinWH) const;
	WHXY GetCurWinXY(int curWinW, int curWinH) const;
	static bool Serialize(const MouseEventBody& inMEBody, Buffer& outStream);
	static bool Deserialize(const Buffer& inStream, MouseEventBody& outMEBody);
};