#pragma 
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "DlProgDlg.h"
#include "CMonitorWin.h"
#include "Singleton.h"
#include "BlockingQueue.h"
#include "Packet.h"
#include "TimerScheduler.h"

#include <atomic>
#include <mutex>
#include <list>
#include <unordered_map>

#define WM_USER_STOPCTRLTHREAD			(WM_USER + 1)
#define WM_USER_STARTENTRYTHREAD		(WM_USER + 2)
#define WM_USER_STOPENTRYTHREAD			(WM_USER + 3)
#define WM_USER_STARTWORKER				(WM_USER + 4)
#define WM_USER_STOPWORKER				(WM_USER + 5)
#define WM_USER_CONNECTSERVER			(WM_USER + 6)

#define WM_USER_RESPONSETIMEOUT			(WM_USER + 100)

#define WM_USER_ERRHANDLER_MINVALID		(WM_USER + 1000)
#define WM_USER_CONNECTIONERR			(WM_USER_ERRHANDLER_MINVALID + 0)
#define WM_USER_UNKNOWNERR				(WM_USER_ERRHANDLER_MINVALID + 1)
#define WM_USER_ERRHANDLER_MAXVALID		(WM_USER + 2000)

class CClientController : public Singleton<CClientController>
{
	friend class Singleton<CClientController>;

public:
	struct ReqInfo
	{
	    Packet* inPacket;
        Packet* outPacket;
		bool isResponse;
        HANDLE handleID;
        UINT msg;
        bool isManualDel;
        ReqInfo(Packet* inPacket, Packet* outPacket, bool isResponse, HANDLE handleID, UINT msg, bool isManualDel = true) :
            inPacket(inPacket), outPacket(outPacket),
            isResponse(isResponse), handleID(handleID), msg(msg),
            isManualDel(isManualDel)
        {}
	};

private:
    struct RegVal {
        Packet* outPacket;
        HANDLE handleID;
        UINT msg;
        size_t taskID;
        RegVal(Packet* outPacket, HANDLE handleID, UINT msg, size_t taskID) :
            outPacket(outPacket), handleID(handleID), msg(msg), taskID(taskID) {}
        RegVal() :
            outPacket(nullptr), handleID(0), msg(0), taskID(0) {}
    };

    struct ReqElem {
        Packet* packet;
        bool isManualDel;
        ReqElem() :
            packet(nullptr), isManualDel(true) {}
        ReqElem(Packet* packet, bool isManualDel) :
            packet(packet), isManualDel(isManualDel) {}
    };

    HandleID GetRegKey() {
        static std::atomic<HandleID> key = 0;
        return key.fetch_add(1);
    }

public:
	int InitController();
	int Invoke(CWnd*& outPMainWnd);

	void UpdataServerAddress(DWORD ip, DWORD port);
	void ConnectServer();

	int SendRequest(ReqInfo& resInfo);

	bool StartDownloadFile(uint32_t fileID, uint64_t totalSize, CString downloadFileName,
							CString savefilePath, bool force = false);

	void StartRemoteMonitor();

protected:
	CClientController();
	~CClientController();
	void CleanupHandles();

	// 线程控制相关
	std::atomic<bool> m_EntryThreadRunning;
	std::atomic<bool> m_WorkerRunning;
	HANDLE m_WorkerStopSyncSignal;
	HANDLE m_RequestWorkerCtrlSignal[2];		// 1 启动相关, 2 结束相关
	HANDLE m_ResponseWorkerCtrlSignal[2];		// 同上
	HANDLE m_EntryCtrlThread;
	unsigned int m_EntryCtrlThreadID;
	static unsigned int __stdcall EntryCtrlThread(void* arg);
	void CtrlThread();
	void PostMsgToCtrlThread(UINT message);
	void StopEntryThread();
	void SafeStartEntryThread();
	void StopWorkerAndWaitRestart();
	void SafeStartWorker();

	// 错误处理相关
	void ClearData();
	void ConnectionErrorHandling();
	void UnknownErrorHandling();

    DWORD m_MainThreadID;
    TimerScheduler m_TimerScheduler;

	// 发送请求线程相关
	BlockingQueue<ReqElem> m_RequestQue;
	HANDLE m_EntryReqWorkerThread;
	unsigned int m_EntryReqWorkerThreadID;
	static unsigned int __stdcall EntryReqWorkerThread(void* arg);
	void RequestWorker();
	void StopRequestWorker();

	// 通知响应线程相关
	std::mutex m_ResRegistryMtx;
	std::unordered_map<HandleID, RegVal> m_ResRegistry;
	HANDLE m_EntryResWorkerThread;
	unsigned int m_EntryResWorkerThreadID;
	static unsigned int __stdcall EntryResWorkerThread(void* arg);
	void ResponseWorker();
	void StopResponseWorker();
	void DispatchResponse(const Packet& resPacket);
    void EraseRegElem(HandleID regKey);

private:
	CRemoteClientDlg m_RCDlg;
	CDlProgDlg m_DPDlg;
	CMonitorWin m_MWDlg;

	DWORD m_server_ip;
	DWORD m_port;
};
