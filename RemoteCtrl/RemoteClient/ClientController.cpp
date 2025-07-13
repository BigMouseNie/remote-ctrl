﻿#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
#include "resource.h"

CClientController::CClientController() :
	m_EntryThreadRunning(false),
	m_WorkerRunning(false),
	m_EntryCtrlThreadID(0),
	m_EntryReqWorkerThreadID(0),
	m_EntryResWorkerThreadID(0),
	m_EntryCtrlThread(NULL),
	m_EntryReqWorkerThread(NULL),
	m_EntryResWorkerThread(NULL),
	m_server_ip(0),
	m_port(0)
{
	m_WorkerStopSyncSignal			= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_RequestWorkerCtrlSignal[0]	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_RequestWorkerCtrlSignal[1]	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_ResponseWorkerCtrlSignal[0]	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_ResponseWorkerCtrlSignal[1]	= CreateEvent(NULL, TRUE, FALSE, NULL);

	m_DPDlg.Create(IDD_DOWNLOAD_FILE, NULL);
	m_MWDlg.Create(IDD_REMOTE_MONITOR, NULL);

    m_MainThreadID = GetCurrentThreadId();
}

CClientController::~CClientController()
{
	if (m_EntryCtrlThread) {
		PostMsgToCtrlThread(WM_USER_STOPENTRYTHREAD);
		PostMsgToCtrlThread(WM_USER_STOPCTRLTHREAD);
	}
	WaitForSingleObject(m_EntryCtrlThread, 100);
	CleanupHandles();
	ClearData();
}

void CClientController::CleanupHandles() {
	if (m_WorkerStopSyncSignal) {
		CloseHandle(m_WorkerStopSyncSignal);
		m_WorkerStopSyncSignal = nullptr;
	}

	for (int i = 0; i < 2; ++i) {
		if (m_RequestWorkerCtrlSignal[i]) {
			CloseHandle(m_RequestWorkerCtrlSignal[i]);
			m_RequestWorkerCtrlSignal[i] = nullptr;
		}
		if (m_ResponseWorkerCtrlSignal[i]) {
			CloseHandle(m_ResponseWorkerCtrlSignal[i]);
			m_ResponseWorkerCtrlSignal[i] = nullptr;
		}
	}

	if (m_EntryCtrlThread) {
		CloseHandle(m_EntryCtrlThread);
		m_EntryCtrlThread = nullptr;
	}

	if (m_EntryReqWorkerThread) {
		CloseHandle(m_EntryReqWorkerThread);
		m_EntryReqWorkerThread = nullptr;
	}

	if (m_EntryResWorkerThread) {
		CloseHandle(m_EntryResWorkerThread);
		m_EntryResWorkerThread = nullptr;
	}
}

/**
 * @return : 小于等于0失败
 */
int CClientController::InitController()	// 创建线程控制线程以及工作线程
{
	m_EntryCtrlThread = (HANDLE)_beginthreadex(NULL, 0, EntryCtrlThread, this, 0, &m_EntryCtrlThreadID);
	SafeStartEntryThread();
	return (m_EntryCtrlThread && m_EntryReqWorkerThread && m_EntryResWorkerThread) ? 1 : -1;
}

/**
 * @brief : 以模态的形式启动主窗口
 */
int CClientController::Invoke(CWnd*& outPMainWnd)
{
	outPMainWnd = &m_RCDlg;
	return m_RCDlg.DoModal();
}

void CClientController::UpdataServerAddress(DWORD ip, DWORD port)
{
	m_server_ip = ip;
	m_port = port;
}

void CClientController::ConnectServer()
{
	PostMsgToCtrlThread(WM_USER_CONNECTSERVER);
}

int CClientController::SendRequest(ReqInfo& reqInfo)
{
    if (!m_WorkerRunning) { return -1; }
    if (reqInfo.isResponse) {
        HandleID regKey = GetRegKey();
        size_t taskID = m_TimerScheduler.post(
            [threadID = m_MainThreadID, msg = WM_USER_RESPONSETIMEOUT, regKey]() {
                ::PostThreadMessage(threadID, msg, regKey, regKey);
            },
            5000);
        RegVal regVal(reqInfo.outPacket, reqInfo.handleID, reqInfo.msg, taskID);
        {
            std::lock_guard<std::mutex> registryLock(m_ResRegistryMtx);
            m_ResRegistry[regKey] = regVal;
        }
        reqInfo.inPacket->header.handleID = regKey;
    }
    m_RequestQue.push({reqInfo.inPacket, reqInfo.isManualDel});
	return 0;
}

unsigned int __stdcall CClientController::EntryCtrlThread(void* arg)	// 线程控制线程如何启动待定
{
	CClientController* thiz = static_cast<CClientController*>(arg);
	thiz->CtrlThread();
	_endthreadex(0);
	return 0;
}

void CClientController::CtrlThread()
{
	MSG msg;
	MSG tmpMsg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		switch (msg.message)
		{
		case WM_USER_STOPCTRLTHREAD:
			return;

		case WM_USER_STARTENTRYTHREAD:
			SafeStartEntryThread();
			break;

		case WM_USER_STOPENTRYTHREAD:
			StopEntryThread();
			break;

		case WM_USER_STARTWORKER:
			SafeStartWorker();
			break;

		case WM_USER_STOPWORKER:
			StopWorkerAndWaitRestart();
			break;

		case WM_USER_CONNECTSERVER:
			StopWorkerAndWaitRestart();
			while (::PeekMessage(&tmpMsg, NULL, WM_USER_ERRHANDLER_MINVALID, WM_USER_ERRHANDLER_MAXVALID, PM_REMOVE)) {}
			ClearData();
			if (CClientSocket::GetInstance()->Connect(m_server_ip, m_port)) {
				SafeStartWorker();
			}
			break;

        case WM_USER_RESPONSETIMEOUT:
            EraseRegElem(msg.wParam);
            break;

		case WM_USER_CONNECTIONERR:
			StopWorkerAndWaitRestart();				// 停止工作
			// 清空错误处理相关信息,线程的断开确保清理之后不会有其他错误消息
			while (::PeekMessage(&tmpMsg, NULL, WM_USER_ERRHANDLER_MINVALID, WM_USER_ERRHANDLER_MAXVALID, PM_REMOVE)) {}
			ConnectionErrorHandling();
			break;

		case WM_USER_UNKNOWNERR:
			StopWorkerAndWaitRestart();
			while (::PeekMessage(&tmpMsg, NULL, WM_USER_ERRHANDLER_MINVALID, WM_USER_ERRHANDLER_MAXVALID, PM_REMOVE)) {}
			UnknownErrorHandling();
			break;

		default:
			break;
		}
	}
}

void CClientController::PostMsgToCtrlThread(UINT message)
{
	::PostThreadMessage(m_EntryCtrlThreadID, message, 0, 0);
}

void CClientController::StopEntryThread()
{
	if (m_WorkerRunning) { StopWorkerAndWaitRestart(); }
	if (m_EntryThreadRunning) {
		m_EntryThreadRunning = false;
		SetEvent(m_RequestWorkerCtrlSignal[0]);	// 需要用启动事件唤醒 EnrtyThread 使其退出
		SetEvent(m_ResponseWorkerCtrlSignal[0]);
		CloseHandle(m_EntryReqWorkerThread); m_EntryReqWorkerThread = NULL;
		CloseHandle(m_EntryResWorkerThread); m_EntryResWorkerThread = NULL;
		m_EntryReqWorkerThreadID = 0;
		m_EntryResWorkerThreadID = 0;
	}
}

void CClientController::SafeStartEntryThread()
{
	StopEntryThread();
	m_EntryThreadRunning = true;
	m_EntryReqWorkerThread = (HANDLE)_beginthreadex(NULL, 0, EntryReqWorkerThread, this, 0, &m_EntryReqWorkerThreadID);
	m_EntryResWorkerThread = (HANDLE)_beginthreadex(NULL, 0, EntryResWorkerThread, this, 0, &m_EntryResWorkerThreadID);
}

void CClientController::SafeStartWorker()
{
	if (m_WorkerRunning) { return; }

	// 重置相关事件
	ResetEvent(m_RequestWorkerCtrlSignal[1]);
	ResetEvent(m_ResponseWorkerCtrlSignal[1]);
	ResetEvent(m_WorkerStopSyncSignal);

	// 当工作函数启动时开始循环
	m_WorkerRunning = true;

	// 通知EntryThread启动工作函数
	SetEvent(m_RequestWorkerCtrlSignal[0]);
	SetEvent(m_ResponseWorkerCtrlSignal[0]);
}

void CClientController::StopWorkerAndWaitRestart()
{
	if (!m_WorkerRunning) { return; }

	// 防止Entry线程重新工作函数
	ResetEvent(m_RequestWorkerCtrlSignal[0]);
	ResetEvent(m_ResponseWorkerCtrlSignal[0]);

	// 使工作函数停止循环(没有退出)
	m_WorkerRunning = false;
	StopRequestWorker();
	StopResponseWorker();

	// 工作函数退出,并等待EntryThread结束信号,保证该函数返回后EntryThread都阻塞在等待重新启动工作函数的位置
	SetEvent(m_WorkerStopSyncSignal);
	WaitForSingleObject(m_RequestWorkerCtrlSignal[1], INFINITE);
	WaitForSingleObject(m_ResponseWorkerCtrlSignal[1], INFINITE);
}

void CClientController::ClearData()
{
    {
        std::lock_guard<std::mutex> lock(m_ResRegistryMtx);
        for (auto& pair : m_ResRegistry) {
            ::PostMessage((HWND)pair.second.handleID, pair.second.msg, 0, 0);
        }
    
        m_ResRegistry.clear();
    }

	m_RequestQue.Clear();
    m_TimerScheduler.Clear();
}

void CClientController::ConnectionErrorHandling()
{
	TRACE("%s : \n", __FUNCTION__);
	ClearData();
}

void CClientController::UnknownErrorHandling()
{
	TRACE("%s : \n", __FUNCTION__);
	ClearData();
}

unsigned int __stdcall CClientController::EntryReqWorkerThread(void* arg)
{
	CClientController* thiz = static_cast<CClientController*>(arg);
	while (thiz->m_EntryThreadRunning) {
		WaitForSingleObject(thiz->m_RequestWorkerCtrlSignal[0], INFINITE);	// 等待启动事件
		if (!thiz->m_EntryThreadRunning) { break; }
		thiz->RequestWorker();
		SetEvent(thiz->m_RequestWorkerCtrlSignal[1]);	// 触发线程结束事件
	}
	_endthreadex(0);
	return 0;
}

void CClientController::RequestWorker()
{
	while (m_WorkerRunning) {
        ReqElem reqElem;
		if (!m_RequestQue.pop(reqElem)) {
			PostMsgToCtrlThread(WM_USER_UNKNOWNERR);
			break;
		}
		if (CClientSocket::GetInstance()->SendPacket(*(reqElem.packet)) < 0) {
			PostMsgToCtrlThread(WM_USER_CONNECTIONERR);
			break;
		}
        if (!reqElem.isManualDel && reqElem.packet) {
            delete reqElem.packet;
        }
	}
	WaitForSingleObject(m_WorkerStopSyncSignal, INFINITE);
}

void CClientController::StopRequestWorker()
{
	m_RequestQue.Release();
	CClientSocket::GetInstance()->Disconnect();
}

unsigned int __stdcall CClientController::EntryResWorkerThread(void* arg)
{
	CClientController* thiz = static_cast<CClientController*>(arg);
	while (thiz->m_EntryThreadRunning) {
		WaitForSingleObject(thiz->m_ResponseWorkerCtrlSignal[0], INFINITE);	// 等待启动事件
		if (!thiz->m_EntryThreadRunning) { break; }
		thiz->ResponseWorker();
		SetEvent(thiz->m_ResponseWorkerCtrlSignal[1]);	// 触发线程结束事件
	}
	_endthreadex(0);
	return 0;
}

void CClientController::ResponseWorker()
{
	Packet recvPacket;
	while(m_WorkerRunning) {
		recvPacket.Clear();
		if (CClientSocket::GetInstance()->ReadPacket(recvPacket) < 0) {
			PostMsgToCtrlThread(WM_USER_CONNECTIONERR);
			break;
		}
		DispatchResponse(recvPacket);
	}
	WaitForSingleObject(m_WorkerStopSyncSignal , INFINITE);
}

void CClientController::StopResponseWorker()
{
	CClientSocket::GetInstance()->Disconnect();
}

void CClientController::DispatchResponse(const Packet& resPacket)
{
	HandleID handleID = resPacket.header.handleID;
    RegVal regVal;
    {
        std::lock_guard<std::mutex> registryLock(m_ResRegistryMtx);
        auto it = m_ResRegistry.find(handleID);
        if (it == m_ResRegistry.end()) { return; }
        regVal = it->second;
        m_ResRegistry.erase(it);
    }
    m_TimerScheduler.Cancel(regVal.taskID);
    *(regVal.outPacket) = resPacket;
    if (!::PostMessage((HWND)regVal.handleID, regVal.msg, 0, 0)) {
        TRACE("%s : %d\n", __FUNCTION__, GetLastError());
    }
}

void CClientController::EraseRegElem(HandleID regKey)
{
    RegVal regVal;
    {
        std::lock_guard<std::mutex> registryLock(m_ResRegistryMtx);
        auto it = m_ResRegistry.find(regKey);
        if (it == m_ResRegistry.end()) { return; }
        regVal = it->second;
        m_ResRegistry.erase(it);
    }
    ::PostMessage((HWND)regVal.handleID, regVal.msg, 0, 0);
}

bool CClientController::StartDownloadFile(uint32_t fileID, uint64_t totalSize, CString downloadFileName,
											CString savefilePath, bool force)
{
	m_DPDlg.ShowWindow(SW_SHOW);
	m_DPDlg.StartDownload(fileID, totalSize, downloadFileName, savefilePath, force);
	return 0;
}

void CClientController::StartRemoteMonitor()
{
	m_MWDlg.StartMonitoring();
}
