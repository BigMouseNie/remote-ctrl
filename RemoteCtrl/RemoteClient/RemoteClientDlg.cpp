
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "Scoped.h"
#include "ClientController.h"

#include <string>
#include <tchar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_DblclkTreeItem(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_addr);
	DDX_Text(pDX, IDC_EDIT_PORT, m_port);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUT_CONN_TEST, &CRemoteClientDlg::OnBnClickedButConnTest)
	ON_BN_CLICKED(IDC_BUT_FILE_LIST, &CRemoteClientDlg::OnBnClickedButFileList)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_FILE_MENU_DOWNLOAD, &CRemoteClientDlg::OnFileMenuDownload)
	ON_COMMAND(ID_FILE_MENU_DEL, &CRemoteClientDlg::OnFileMenuDel)
	ON_COMMAND(ID_FILE_MENU_OPEN, &CRemoteClientDlg::OnFileMenuOpen)
	ON_BN_CLICKED(IDC_BUT_REMOTE_MONITOR, &CRemoteClientDlg::OnBnClickedButRemoteMonitor)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
    ON_MESSAGE(WM_USER_CONNECTSERVERANDTEST, &CRemoteClientDlg::OnConnectServer)
    ON_MESSAGE(WM_USER_DISKPARTINFO, &CRemoteClientDlg::OnDiskPartInfo)
    ON_MESSAGE(WM_USER_FILELIST, &CRemoteClientDlg::OnFileList)
    ON_MESSAGE(WM_USER_DOWNLOADFILE, &CRemoteClientDlg::OnDownloadFile)
    ON_MESSAGE(WM_USER_OPENFILE, &CRemoteClientDlg::OnOpenFile)
    ON_MESSAGE(WM_USER_DELFILE, &CRemoteClientDlg::OnDelFile)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_serv_addr = 0x7F000001;
	m_port = 8081;
	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString CRemoteClientDlg::GetTreeItemFullPath(HTREEITEM hItem)
{
	CString fullPath;
	while (hItem != NULL)
	{
		CString name = m_tree.GetItemText(hItem);
		if (!fullPath.IsEmpty())
			fullPath = name + _T("\\") + fullPath;
		else
			fullPath = name;

		hItem = m_tree.GetParentItem(hItem);
	}
	return fullPath;
}

void CRemoteClientDlg::DeleteAllChildren(HTREEITEM hParent)
{
	HTREEITEM hChild = m_tree.GetChildItem(hParent);  // 获取第一个子节点
	while (hChild != NULL)
	{
		HTREEITEM hNext = m_tree.GetNextSiblingItem(hChild);  // 提前保存下一个兄弟节点
		m_tree.DeleteItem(hChild);  // 删除当前子节点
		hChild = hNext;  // 继续下一个
	}
}

void CRemoteClientDlg::UpdateFileTree(HTREEITEM hParent, const FileListBody& flBody)
{
	const FileNameLen* fnlArr = reinterpret_cast<const FileNameLen*>(flBody.fileNameLens.GetReadPtr());
	const FileType* ftArr = reinterpret_cast<const FileType*>(flBody.fileTypes.GetReadPtr());
	const char* fnBuf = flBody.fileNameBuffer.GetReadPtr();
	char szFileName[1024];
	size_t offset = 0;
	for (FileCnt i = 0; i < flBody.fileCnt; ++i) {
		memcpy(szFileName, fnBuf + offset, fnlArr[i]);
		szFileName[fnlArr[i]] = '\0';
		offset += fnlArr[i];
		if (ftArr[i] != FileType::FT_DIR) {
			m_list.InsertItem(0, szFileName);
		}
		else {
			m_tree.InsertItem(szFileName, hParent);
		}
	}
}

void CRemoteClientDlg::OnBnClickedButConnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::GetInstance()->ConnectServer();
	Sleep(100);

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_CONNECTSERVERANDTEST, rp)) {
        MessageBox(_T("未知错误！"), _T("连接测试"), MB_OK | MB_ICONERROR);
        return;
    }

	char testStr[] = "echo test!";
    CPacketHandler::BuildPacket(*(rp.inPacket), testStr, strlen(testStr) + 1, ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_TEST, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_CONNECTSERVERANDTEST);
    int count = 0;
	while (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
		if (count++ < 3) { Sleep(100); continue; }
		TRACE("%s : %s\n", __FUNCTION__, _T("Send Request falid(retry 3 times)!"));
		MessageBox(_T("连接失败！"), _T("连接测试"), MB_OK | MB_ICONERROR);
        TRACE("%s : %s\n", __FUNCTION__, _T("SendRequest Err!"));
        m_MapPacket.PutResourcePacket(WM_USER_CONNECTSERVERANDTEST);
        return;
	}
}

afx_msg LRESULT CRemoteClientDlg::OnConnectServer(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_CONNECTSERVERANDTEST, rp)) {
        MessageBox(_T("未知错误！"), _T("连接测试"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        TRACE("%s : %s\n", __FUNCTION__, _T("Empty Err!"));
        MessageBox(_T("连接失败！"), _T("连接测试"), MB_OK | MB_ICONERROR);
    }
    else {
        TRACE("%s : %s\n", __FUNCTION__, rp.outPacket->body.GetReadPtr());
        MessageBox(_T("连接成功！"), _T("连接测试"), MB_OK | MB_ICONINFORMATION);
    }

    m_MapPacket.PutResourcePacket(WM_USER_CONNECTSERVERANDTEST);
    return 0;
}

void CRemoteClientDlg::OnBnClickedButFileList()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tree.DeleteAllItems();
	m_list.DeleteAllItems();

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_DISKPARTINFO, rp)) {
        MessageBox(_T("未知错误！"), _T("磁盘信息"), MB_OK | MB_ICONERROR);
        return;
    }

    CPacketHandler::BuildPacketHeaderInPacket(*(rp.inPacket), ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_DISK_PART, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_DISKPARTINFO);
    if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
        MessageBox(_T("获取磁盘信息失败！"), _T("磁盘信息"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DISKPARTINFO);
        return;
    }
}

afx_msg LRESULT CRemoteClientDlg::OnDiskPartInfo(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_DISKPARTINFO, rp)) {
        MessageBox(_T("未知错误！"), _T("磁盘信息"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        MessageBox(_T("获取磁盘信息失败！"), _T("磁盘信息"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DISKPARTINFO);
        return 0;
    }

    uint32_t diskPart = 0;
    if (
        (!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_DISK_PART)) ||
        ((rp.outPacket->header.statusCode & StatusCode::SC_ERR) != 0) ||
        ((!rp.outPacket->body.TryPeekExact(reinterpret_cast<char*>(&diskPart), sizeof(diskPart))))
        )
    {
        MessageBox(_T("获取文件列表失败！"), _T("获取文件列表"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DISKPARTINFO);
        return 0;
    }
    uint32_t flag = 1;
    char diskPartStr[] = "_:";
    for (int i = 0; i < 26; ++i) {
        if (diskPart & flag) {
            diskPartStr[0] = 'A' + i;
            m_tree.InsertItem(diskPartStr);
        }
        flag <<= 1;
    }
    m_MapPacket.PutResourcePacket(WM_USER_DISKPARTINFO);
    return 0;
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	// 获取 MousePoint
	*pResult = 0;
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);  // 转换为控件的客户坐标
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) {
		return;
	}

	// 记录双击的树节点,清空树节点下的子节点和清空m_list,准备更新数据
	m_DblclkTreeItem = hTreeSelected;
	m_list.DeleteAllItems();
	DeleteAllChildren(hTreeSelected);
	CString fullPathStr = GetTreeItemFullPath(hTreeSelected);
	if (!fullPathStr.IsEmpty() && fullPathStr.GetAt(fullPathStr.GetLength() - 1) == _T('\\')) {
		fullPathStr.Delete(fullPathStr.GetLength() - 1);
	}

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_FILELIST, rp)) {
        MessageBox(_T("未知错误！"), _T("文件列表"), MB_OK | MB_ICONERROR);
        return;
    }

    CPacketHandler::BuildPacket(*(rp.inPacket), fullPathStr, strlen(fullPathStr), ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_LIST_FILE, StatusCode::SC_NONE);
    
    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_FILELIST);
	if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
		MessageBox(_T("获取文件列表失败！"), _T("文件列表"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_FILELIST);
		return;
	}
}

afx_msg LRESULT CRemoteClientDlg::OnFileList(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_FILELIST, rp)) {
        MessageBox(_T("未知错误！"), _T("文件列表"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        MessageBox(_T("获取文件列表失败！"), _T("文件列表"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_FILELIST);
        return 0;
    }

    FileListBody flBody;
    if (
        (!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_LIST_FILE)) ||
        ((rp.outPacket->header.statusCode & StatusCode::SC_ERR) != 0) ||
        (!(FileListBody::Deserialize(rp.outPacket->body, flBody)))
        )
    {
        MessageBox(_T("获取文件列表失败！"), _T("获取文件列表"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_FILELIST);
        return 0;
    }
    UpdateFileTree(m_DblclkTreeItem, flBody);
    m_tree.Expand(m_DblclkTreeItem, TVE_EXPAND);
    m_MapPacket.PutResourcePacket(WM_USER_FILELIST);
    return 0;
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_list.ScreenToClient(&ptList);
	int listSelected = m_list.HitTest(ptList);
	if (listSelected < 0) {
		return;
	}
	CMenu fMenu;
	fMenu.LoadMenu(IDR_FILE_MENU);
	CMenu* subMenu = fMenu.GetSubMenu(0);
	if (subMenu != NULL) {
		subMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::OnFileMenuDownload()
{
	// TODO: 在此添加命令处理程序代码
	int listSelected = m_list.GetSelectionMark();
	if (listSelected < 0) { return; }
	CString listItemText = m_list.GetItemText(listSelected, 0);
	CFileDialog dlg(FALSE, _T("txt"), listItemText,
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ENABLESIZING,
		_T("All Files (*.*)\0*.*\0\0"), this);
	if (dlg.DoModal() != IDOK) {
		return;
	}
	TRACE("%s : %s %s\n", __FUNCTION__, _T("save file path : "), dlg.GetPathName());
	CString dlPathCStr = GetTreeItemFullPath(m_DblclkTreeItem);
	dlPathCStr.AppendChar('\\');
	dlPathCStr.Append(listItemText);
	TRACE("%s : %s %s\n", __FUNCTION__, _T("download file path : "), dlPathCStr);
	CString savePathCStr = dlg.GetPathName();

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_DOWNLOADFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("文件列表"), MB_OK | MB_ICONERROR);
        return;
    }

	ReqFileBody rfBody;
	rfBody.ctrlCode = ReqFileBody::RFB_CC_BEGIN;
	rfBody.filePathLen = strlen(dlPathCStr);
	rfBody.filePath.Write(dlPathCStr, strlen(dlPathCStr));
	if (!ReqFileBody::Serialize(rfBody, rp.inPacket->body)) {
		MessageBox(_T("下载失败！"), _T("文件下载"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DOWNLOADFILE);
		return;
	}

    CPacketHandler::BuildPacketHeaderInPacket(*(rp.inPacket), ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_DOWNLOAD_FILE, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_DOWNLOADFILE);

    if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
        MessageBox(_T("下载失败！"), _T("文件下载"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DOWNLOADFILE);
        return;
    }

    m_DownloadFilePath[0] = dlPathCStr;
    m_DownloadFilePath[1] = savePathCStr;
}

afx_msg LRESULT CRemoteClientDlg::OnDownloadFile(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_DOWNLOADFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("文件下载"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        MessageBox(_T("下载失败！"), _T("文件下载"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DOWNLOADFILE);
        return 0;
    }

    FileBody fBody;
    if (
        (!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_DOWNLOAD_FILE)) ||
        ((rp.outPacket->header.statusCode & StatusCode::SC_ERR) != 0) ||
        (!FileBody::Deserialize(rp.outPacket->body, fBody))
       )
    {
        MessageBox(_T("下载失败！"), _T("文件下载"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DOWNLOADFILE);
        return 0;
    }

    m_MapPacket.PutResourcePacket(WM_USER_DOWNLOADFILE);
    CClientController::GetInstance()->StartDownloadFile(fBody.fileID, fBody.totalSize,
        m_DownloadFilePath[0], m_DownloadFilePath[1]);
    return 0;
}

void CRemoteClientDlg::OnFileMenuDel()
{
	// TODO: 在此添加命令处理程序代码
	m_CurReqDelItem = m_list.GetSelectionMark();
	if (m_CurReqDelItem < 0) { return; }
	CString listItemText = m_list.GetItemText(m_CurReqDelItem, 0);
	CString fullPathStr = GetTreeItemFullPath(m_DblclkTreeItem);
	fullPathStr.AppendChar('\\');
	fullPathStr.Append(listItemText);
	TRACE("%s : %s %s\n", __FUNCTION__, _T("delete file path : "), fullPathStr);

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_DELFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("删除文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DELFILE);
        return;
    }

    CPacketHandler::BuildPacket(*(rp.inPacket), fullPathStr, strlen(fullPathStr), ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_DEL_FILE, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_DELFILE);

    if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
        MessageBox(_T("删除文件失败！"), _T("删除文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DELFILE);
        return;
    }
}

afx_msg LRESULT CRemoteClientDlg::OnDelFile(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_DELFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("删除文件"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        MessageBox(_T("删除文件失败！"), _T("删除文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DELFILE);
        return 0;
    }

    if (
        (!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_DEL_FILE)) ||
        ((rp.outPacket->header.statusCode & StatusCode::SC_ERR) != 0)
        )
    {
        MessageBox(_T("删除文件失败！"), _T("删除文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_DELFILE);
        return 0;
    }
    m_list.DeleteItem(m_CurReqDelItem);
    MessageBox(_T("删除文件成功！"), _T("删除文件"), MB_OK | MB_ICONINFORMATION);
    m_MapPacket.PutResourcePacket(WM_USER_DELFILE);
    return 0;
}

void CRemoteClientDlg::OnFileMenuOpen()
{
	// TODO: 在此添加命令处理程序代码
	int listSelected = m_list.GetSelectionMark();
	if (listSelected < 0) { return; }
	CString listItemText = m_list.GetItemText(listSelected, 0);
	CString fullPathStr = GetTreeItemFullPath(m_DblclkTreeItem);
	fullPathStr.AppendChar('\\');
	fullPathStr.Append(listItemText);
	TRACE("%s : %s %s\n", __FUNCTION__, _T("open file path : "), fullPathStr);

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_OPENFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("打开文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_OPENFILE);
        return;
    }

    CPacketHandler::BuildPacket(*(rp.inPacket), fullPathStr, strlen(fullPathStr), ChecksumType::CT_SUM,
        ReqRes::RR_REQUEST | CommandType::CMD_RUN_FILE, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_OPENFILE);

    if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
        MessageBox(_T("打开文件失败！"), _T("打开文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_OPENFILE);
        return;
    }
}

afx_msg LRESULT CRemoteClientDlg::OnOpenFile(WPARAM wParam, LPARAM lParam)
{
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.GetResourcePacket(WM_USER_OPENFILE, rp)) {
        MessageBox(_T("未知错误！"), _T("打开文件"), MB_OK | MB_ICONERROR);
        return 0;
    }

    if (rp.outPacket->Empty()) {
        MessageBox(_T("文件打开成功！"), _T("打开文件"), MB_OK | MB_ICONERROR);
        m_MapPacket.PutResourcePacket(WM_USER_OPENFILE);
        return 0;
    }

    if (
        (!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_RUN_FILE)) ||
        ((rp.outPacket->header.statusCode & StatusCode::SC_ERR) != 0)
        )
    {
        MessageBox(_T("打开文件失败！"), _T("打开文件"), MB_OK | MB_ICONERROR);
    }
    else
    {
        MessageBox(_T("文件打开成功！"), _T("打开文件"), MB_OK | MB_ICONINFORMATION);
    }
    m_MapPacket.PutResourcePacket(WM_USER_OPENFILE);
    return 0;
}

void CRemoteClientDlg::OnBnClickedButRemoteMonitor()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::GetInstance()->StartRemoteMonitor();
}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController::GetInstance()->UpdataServerAddress(m_serv_addr, m_port);
}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController::GetInstance()->UpdataServerAddress(m_serv_addr, m_port);
}
