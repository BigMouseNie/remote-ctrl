#pragma once

#include "Packet.h"
#include "Body.h"
#include "CallBack.h"
#include "LockDialog.h"

#include <string>
#include <mutex>
#include <map>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <unordered_map>

class FileDownloadSession;
class CCmdHandler
{
public:
	static bool Init();
	static void Release();
	static bool SetChecksumType(ChecksumType csType);
	static ChecksumType GetChecksumType();

	/**
	 * @return : -1需要做断开处理
	 */
	static int DealPacket(const Packet& inPacket, Packet** ppOutPacket);

private:
	static int TEST_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int DISK_PART_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int LIST_FILE_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int DOWNLOAD_FILE_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int RUN_FILE_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int DEL_FILE_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int SEND_SCREEN_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int MOUSE_EVENT_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int LOCK_MACHINE_handler(const Packet& inPacket, Packet** ppOutPacket);
	static int UNLOCK_MACHINE_handler(const Packet& inPacket, Packet** ppOutPacket);

private:
	// 下载文件相关
	static void DOWNLOAD_FILE_BEGIN_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode);
	static void DOWNLOAD_FILE_STOP_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode);
	static void DOWNLOAD_FILE_NEXT_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode);
	static void DOWNLOAD_FILE_PREV_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode);
	static std::condition_variable NoWait;
	static std::mutex FDSMapMtx;
	static const size_t MaxidleSec;
	static std::map<uint32_t, FileDownloadSession*> FDSMap;
	static HANDLE FDSRecycleThreadHANDLE;
	static unsigned int __stdcall FDSRecycleThread(void* arg);

	// GDI+
	static ULONG_PTR g_gdiplusToken;

private:
	static ChecksumType DefCsType;
	static bool IsLockedMachine;
	static CLockDialog lockDlg;
	static unsigned int lockDlgTID;
	static const std::unordered_map<uint32_t, DealPacketCallBack> IdxCmdHandler;
	static std::atomic<bool> isRelese;

private:
	explicit CCmdHandler() = default;
	static void SimulateMouseClick(const MouseEventBody& meb);
	static unsigned __stdcall ThreadLockMachine(void* arg);
};

class FileDownloadSession
{
public:
	FileDownloadSession() = default;
	~FileDownloadSession() {
		if (pf) {
			fclose(pf);
			pf = nullptr;
		}
	}

	FileDownloadSession(const FileDownloadSession&) = delete;
	FileDownloadSession& operator=(const FileDownloadSession&) = delete;
	FileDownloadSession(FileDownloadSession&&) = delete;
	FileDownloadSession& operator=(FileDownloadSession&&) = delete;

	bool OpenFile(const char* filePath);
	bool GetBaseFileInfo(FileBody& fbBody) const;
	bool GetFileBodyToBuffer(Buffer& fBody, bool getPreChunk = false);
	void SetChunkSize(uint32_t cSize);
	uint32_t GetFileID();
	time_t GetLastActiveTime();
	bool IsFinished();
	void Close();

private:
	size_t ReadChunk(char* outChunk);
	void ReadChunk(Buffer& outChunk);
	size_t ReadPreChunk(char* outChunk);
	void ReadPreChunk(Buffer& outChunk);

	FILE* pf = nullptr;
	uint32_t fileID;
	std::string filePath;
	uint32_t chunkSize = 2048;
	uint64_t lastOffset = 0;
	uint64_t offset = 0;
	uint64_t totalSize = 0;
	time_t lastActiveTime = 0;    // 最后活动时间（超时清理用）
	bool isFinished = false;       // 是否下载完成
	static const uint32_t minChunkSize;
	static uint32_t nextFileID;
};
