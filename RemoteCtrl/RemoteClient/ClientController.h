#pragma 
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "DlProgDlg.h"
#include "CMonitorWin.h"
#include "Singleton.h"
#include "BlockingQueue.h"

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
	    const Packet* pPacket;
		bool isResponse;
		ReqInfo(const Packet* pPacket, bool isResponse = true) : pPacket(pPacket), isResponse(isResponse) {}
	};

	struct ResInfo
	{
		HANDLE hEvent;
		Packet** ppPacket;
		ResInfo() : hEvent(NULL), ppPacket(NULL) {}
		ResInfo(HANDLE hEvent, Packet** ppPacket) : hEvent(hEvent), ppPacket(ppPacket) {}
	};
public:
	int InitController();
	int Invoke(CWnd*& outPMainWnd);

	void UpdataServerAddress(DWORD ip, DWORD port);
	void ConnectServer();

	int SendRequest(ReqInfo resInfo);
	int RegisterResponse(uint32_t handleID, Packet** ppOutPacket, HANDLE hEvent);

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
	void ClearThreadData();
	void ConnectionErrorHandling();
	void UnknownErrorHandling();

	// 发送请求线程相关
	BlockingQueue<ReqInfo> m_BlockingQue;
	HANDLE m_EntryReqWorkerThread;
	unsigned int m_EntryReqWorkerThreadID;
	static unsigned int __stdcall EntryReqWorkerThread(void* arg);
	void RequestWorker();
	void StopRequestWorker();

	// 通知响应线程相关
	std::mutex m_ResRegistryMtx;
	std::unordered_map<uint32_t, std::list<ResInfo>> m_ResRegistry;
	HANDLE m_EntryResWorkerThread;
	unsigned int m_EntryResWorkerThreadID;
	static unsigned int __stdcall EntryResWorkerThread(void* arg);
	void ResponseWorker();
	void StopResponseWorker();
	void DispatchResponse(const Packet& resPacket);

private:
	CRemoteClientDlg m_RCDlg;
	CDlProgDlg m_DPDlg;
	CMonitorWin m_MWDlg;

	DWORD m_server_ip;
	DWORD m_port;
};
