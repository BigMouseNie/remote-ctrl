#pragma once

#include "Packet.h"
#include "Body.h"
#include "resource.h"
#include "MapPacket.h"

#include <atomic>

#define WM_USER_STARTREMOTEMONITOR		(WM_USER + 1)
#define WM_USER_STOPREMOTEMONITOR		(WM_USER + 2)
#define WM_USER_ENABLEWINCTRL			(WM_USER + 3)
#define WM_USER_DISABLEWINCTRL			(WM_USER + 4)
#define WM_USER_FRAMEDATACOMING         (WM_USER + 5)
#define WM_USER_LOCKMACHINE             (WM_USER + 6)
#define WM_USER_UNLOCKMACHINE           (WM_USER + 7)

#define WM_USER_REMOTEMONITOR_ERR		(WM_USER + 1000)

// CMonitorWin 对话框

class CMonitorWin : public CDialogEx
{
	DECLARE_DYNAMIC(CMonitorWin)

public:
	CMonitorWin(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CMonitorWin();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTE_MONITOR };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	void InterPosToMEB(CPoint inGPos, MouseEventBody& outMEBody);
	void DealMouseEvent(CPoint point, MouseEventBody& meb);

private:
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButLockmachine();
	afx_msg void OnBnClickedButUnlockmachine();
	afx_msg void OnClose();
	afx_msg LRESULT OnStartRemoteMonitor(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnStopRemoteMonitor(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEnableWinCtrl(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDisableWinCtrl(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFrameDataComing(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoteMonitorErr(WPARAM wParam, LPARAM lParam);


public:
	void StartMonitoring();

private:
    MapPacket m_MapPacket;
	CButton m_LockMachine;
	CButton m_UnlockMachine;
	CStatic m_picture;
	CImage m_cachedImage;     // 缓存远程图像
	CRect m_pictureRect;      // 控件的区域（用于缩放绘制）
	void DrawFrame(const Buffer& frameBuf);
	
	// 鼠标的远程控制(请使用消息控制该参数)
	bool m_EnableWinCtrl;

	// 监控线程相关
    HANDLE m_HEvent;
	std::atomic<bool> m_IsMonitoring;
	std::atomic<bool> m_StopMonitoring;
	unsigned int m_EntryRemoteMonitorThreadID;
	HANDLE m_EntryRemoteMonitorThread;
	static unsigned int __stdcall EntryRemoteMonitorThread(void* arg);
	void RemoteMonitor();
};
