#pragma once

#include <atomic>
#include "resource.h"
#include "MapPacket.h"

#define WM_USER_STARTDOWNLOAD	        (WM_USER + 1)
#define WM_USER_STOPDOWNLOAD	        (WM_USER + 2)
#define WM_USER_DOWNLOADPROG	        (WM_USER + 3)
#define WM_USER_DOWNLOADFINISH	        (WM_USER + 4)
#define WM_USER_DOWNLOADDATACOMING      (WM_USER + 5)

#define WM_USER_DOWNLOADERR_MINVALID		(WM_USER + 1000)
#define WM_USER_DOWNLOAD_OPENERR			(WM_USER_DOWNLOADERR_MINVALID + 1)
#define WM_USER_DOWNLOAD_ERR				(WM_USER_DOWNLOADERR_MINVALID + 2)
#define WM_USER_DOWNLOADERR_MAXVALID		(WM_USER + 2000)

// CDlProgDlg 对话框

class CDlProgDlg : public CDialog
{
	DECLARE_DYNAMIC(CDlProgDlg)

public:
	CDlProgDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlProgDlg();

	void StartDownload(uint32_t fileID, uint64_t totalSize, CString downloadFileName, CString savefilePath, bool force = false);
	void StopDownload();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DOWNLOAD_FILE };
#endif

protected:
    MapPacket m_MapPacket;
    HANDLE m_HEvent;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg LRESULT OnStartDownload(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnStopDownload(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadProg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadFinish(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDownloadDataComing(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadErr(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_prog_dlfile;

private:
	void ResetProg();
	void ClearDownloadFileInfo();
	std::atomic<bool> m_isDownloading;

	// 下载文件的信息
	uint32_t m_fileID;
	std::atomic<uint64_t> m_offset;
	uint64_t m_totalSize;
	CString m_downloadFileName;
	CString m_saveFilePath;

	// 下载文件线程相关
	std::atomic<bool> m_StopDownloading;
	unsigned int m_EntryDownloadFileThreadID;
	HANDLE m_EntryDownloadFileThread;
	static unsigned int __stdcall EntryDownloadFileThread(void* arg);
	void DownloadFile();
	bool StopDownloadFileThread();
};
