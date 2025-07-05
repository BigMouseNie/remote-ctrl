
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "Body.h"
#include "Packet.h"

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

public:
	DWORD m_serv_addr;
	DWORD m_port;
	CListCtrl m_list;
	CTreeCtrl m_tree;
	afx_msg void OnBnClickedButConnTest();
	afx_msg void OnBnClickedButFileList();
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFileMenuDownload();
	afx_msg void OnFileMenuDel();
	afx_msg void OnFileMenuOpen();
	afx_msg void OnBnClickedButRemoteMonitor();
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
};
