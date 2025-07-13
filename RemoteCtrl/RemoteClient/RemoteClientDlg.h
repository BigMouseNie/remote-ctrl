
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "Body.h"
#include "Packet.h"

#include "MapPacket.h"

#define WM_USER_CONNECTSERVERANDTEST	(WM_USER + 1)
#define WM_USER_DISKPARTINFO            (WM_USER + 2)
#define WM_USER_FILELIST                (WM_USER + 3)
#define WM_USER_DOWNLOADFILE            (WM_USER + 4)
#define WM_USER_OPENFILE                (WM_USER + 5)
#define WM_USER_DELFILE                 (WM_USER + 6)
    

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

protected:
	CString GetTreeItemFullPath(HTREEITEM hItem);
	void DeleteAllChildren(HTREEITEM hParent);
	void UpdateFileTree(HTREEITEM hParent, const FileListBody& flBody);

	HTREEITEM m_DblclkTreeItem;
    MapPacket m_MapPacket;
    CString m_DownloadFilePath[2]; // 0:目标路径, 1:保存路径
    int m_CurReqDelItem;

    afx_msg LRESULT OnConnectServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDiskPartInfo(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFileList(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDownloadFile(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnOpenFile(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDelFile(WPARAM wParam, LPARAM lParam);

public:
	DWORD m_serv_addr;
	DWORD m_port;
	CListCtrl m_list;
	CTreeCtrl m_tree;
	afx_msg void OnBnClickedButConnTest();
	afx_msg void OnBnClickedButFileList();
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFileMenuDownload();
	afx_msg void OnFileMenuDel();
	afx_msg void OnFileMenuOpen();
	afx_msg void OnBnClickedButRemoteMonitor();
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
};
