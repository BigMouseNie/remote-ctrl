// CMonitorWin.cpp: 实现文件
//

#include "pch.h"
#include "CMonitorWin.h"
#include "afxdialogex.h"
#include "Scoped.h"
#include "ClientController.h"

// CMonitorWin 对话框

IMPLEMENT_DYNAMIC(CMonitorWin, CDialogEx)

CMonitorWin::CMonitorWin(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTE_MONITOR, pParent),
	m_IsMonitoring(false),
	m_StopMonitoring(true),
	m_EntryRemoteMonitorThreadID(0),
	m_EntryRemoteMonitorThread(NULL)
{

}

CMonitorWin::~CMonitorWin()
{
	if (!m_cachedImage.IsNull()) {
		m_cachedImage.Destroy();
	}
}

void CMonitorWin::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MONITOR_INTERFACE, m_picture);
	DDX_Control(pDX, IDC_BUT_LOCKMACHINE, m_LockMachine);
	DDX_Control(pDX, IDC_BUT_UNLOCKMACHINE, m_UnlockMachine);
}


BEGIN_MESSAGE_MAP(CMonitorWin, CDialogEx)
	ON_WM_MBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BUT_LOCKMACHINE, &CMonitorWin::OnBnClickedButLockmachine)
	ON_BN_CLICKED(IDC_BUT_UNLOCKMACHINE, &CMonitorWin::OnBnClickedButUnlockmachine)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_USER_STARTREMOTEMONITOR, &CMonitorWin::OnStartRemoteMonitor)
	ON_MESSAGE(WM_USER_STOPREMOTEMONITOR, &CMonitorWin::OnStopRemoteMonitor)
	ON_MESSAGE(WM_USER_ENABLEWINCTRL, &CMonitorWin::OnEnableWinCtrl)
	ON_MESSAGE(WM_USER_DISABLEWINCTRL, &CMonitorWin::OnDisableWinCtrl)
	ON_MESSAGE(WM_USER_REMOTEMONITOR_ERR, &CMonitorWin::OnRemoteMonitorErr)
END_MESSAGE_MAP()


void CMonitorWin::InterPosToMEB(CPoint inGPos, MouseEventBody& outMEBody)
{
	CRect picRect;
	m_picture.GetWindowRect(&picRect);
	outMEBody.senderWinWH.HY = picRect.Height();
	outMEBody.senderWinWH.WX = picRect.Width();
	outMEBody.relativeXY.WX = inGPos.x;
	outMEBody.relativeXY.HY = inGPos.y;
}

// CMonitorWin 消息处理程序

BOOL CMonitorWin::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_picture.GetWindowRect(&m_pictureRect);
	ScreenToClient(&m_pictureRect);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CMonitorWin::StartMonitoring()
{
	if (m_IsMonitoring) { return; }
	PostMessage(WM_USER_STARTREMOTEMONITOR);
}

void CMonitorWin::DrawFrame(const Buffer& frameBuf)
{
	IStream* pStream = SHCreateMemStream((const BYTE*)(frameBuf.GetReadPtr()), frameBuf.Readable());
	if (!pStream) return;

	m_cachedImage.Destroy();

	if (SUCCEEDED(m_cachedImage.Load(pStream))) {
		CClientDC dc(&m_picture);
		dc.SetStretchBltMode(HALFTONE);
		m_cachedImage.Draw(dc, m_pictureRect);
	}
	pStream->Release();
}

unsigned int __stdcall CMonitorWin::EntryRemoteMonitorThread(void* arg)
{
	CMonitorWin* thiz = reinterpret_cast<CMonitorWin*>(arg);
	thiz->RemoteMonitor();
	return 0;
}

void CMonitorWin::RemoteMonitor()
{
	m_IsMonitoring = true;
	Packet inPacket, outPacket;
	DWORD handleID = ::GetCurrentThreadId();
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Scoped<HANDLE, decltype(&CloseHandle)> scoped(hEvent, &CloseHandle);
	Packet* pOutPacket = &outPacket; Packet** ppOutPacket = &pOutPacket;
	CPacketHandler::BuildPacketHeaderInPacket(inPacket, StatusCode::SC_NONE, ChecksumType::CT_NONE,
		handleID, ReqRes::RR_REQUEST | CommandType::CMD_SEND_SCREEN);
	PostMessage(WM_USER_ENABLEWINCTRL, 0, 0);
	while (!m_StopMonitoring) {
		if (CClientController::GetInstance()->RegisterResponse(handleID, ppOutPacket, hEvent) < 0) {
			PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
			break;
		}

		if (CClientController::GetInstance()->SendRequest(&inPacket) < 0) {
			PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
			break;
		}

		WaitForSingleObject(hEvent, INFINITE);
		ResetEvent(hEvent);

		if (
			((*ppOutPacket) == nullptr) ||
			(!CPacketHandler::ValidatePacket(outPacket, ReqRes::RR_RESPONSE | CommandType::CMD_SEND_SCREEN))
			)
		{
			PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
			break;
		}

		DrawFrame(outPacket.body);
		outPacket.Clear();
		Sleep(200);
	}
	PostMessage(WM_USER_DISABLEWINCTRL, 0, 0);
	m_IsMonitoring = false;
}

// 自定义事件处理
afx_msg LRESULT CMonitorWin::OnStartRemoteMonitor(WPARAM wParam, LPARAM lParam)
{
	if (m_IsMonitoring) { return 0; }
	m_StopMonitoring = false;
	m_EntryRemoteMonitorThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, EntryRemoteMonitorThread,
																		this, 0, &m_EntryRemoteMonitorThreadID));
	if (m_EntryRemoteMonitorThread == NULL) {
		PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
	}
	ShowWindow(SW_SHOW);
	return 0;
}

afx_msg LRESULT CMonitorWin::OnStopRemoteMonitor(WPARAM wParam, LPARAM lParam)
{
	ShowWindow(SW_HIDE);
	if (!m_IsMonitoring) { return 0; }
	m_StopMonitoring = true;
	if (m_EntryRemoteMonitorThread) {
		WaitForSingleObject(m_EntryRemoteMonitorThread, INFINITE);
		CloseHandle(m_EntryRemoteMonitorThread);
		m_EntryRemoteMonitorThread = NULL;
	}
	return 0;
}

afx_msg LRESULT CMonitorWin::OnEnableWinCtrl(WPARAM wParam, LPARAM lParam)
{
	m_EnableWinCtrl = true;
	m_LockMachine.EnableWindow(TRUE);
	m_UnlockMachine.EnableWindow(TRUE);
	return 0;
}

afx_msg LRESULT CMonitorWin::OnDisableWinCtrl(WPARAM wParam, LPARAM lParam)
{
	m_EnableWinCtrl = false;
	m_LockMachine.EnableWindow(FALSE);
	m_UnlockMachine.EnableWindow(FALSE);
	return 0;
}

afx_msg LRESULT CMonitorWin::OnRemoteMonitorErr(WPARAM wParam, LPARAM lParam)
{
	MessageBox(_T("远程监控发生错误失败！"), _T("远程监控"), MB_OK | MB_ICONERROR);
	PostMessage(WM_USER_STOPREMOTEMONITOR, 0, 0);
	return 0;
}

// 关闭事件处理
void CMonitorWin::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	PostMessage(WM_USER_STOPREMOTEMONITOR, 0, 0);
}

// 鼠标事件处理
void CMonitorWin::DealMouseEvent(CPoint point, MouseEventBody& meb)
{
	CRect picRect;
	m_picture.GetWindowRect(&picRect);
	ScreenToClient(&picRect);
	if (!picRect.PtInRect(point))
	{
		return;
	}

	point.Offset(-picRect.left, -picRect.top);
	InterPosToMEB(point, meb);
	Packet* pPacket = new Packet;
	if (!MouseEventBody::Serialize(meb, pPacket->body)) {
		TRACE("%s : %s\n", __FUNCTION__, "Serialize failed!");
		return;
	}

	CPacketHandler::BuildPacketHeaderInPacket(*pPacket, StatusCode::SC_NONE,
		ChecksumType::CT_NONE, 0, ReqRes::RR_REQUEST | CommandType::CMD_MOUSE_EVENT);

	CClientController::ReqInfo rInfo(pPacket, false);
	CClientController::GetInstance()->SendRequest(rInfo);
	return;
}

void CMonitorWin::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_MIDBTN;
		meb.mba = MouseBtnAct::MBA_DOUBLE;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnMButtonDblClk(nFlags, point);
}

void CMonitorWin::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_MIDBTN;
		meb.mba = MouseBtnAct::MBA_DOWN;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnMButtonDown(nFlags, point);
}

void CMonitorWin::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_MIDBTN;
		meb.mba = MouseBtnAct::MBA_UP;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnMButtonUp(nFlags, point);
}

void CMonitorWin::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_LEFTBTN;
		meb.mba = MouseBtnAct::MBA_DOUBLE;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CMonitorWin::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_LEFTBTN;
		meb.mba = MouseBtnAct::MBA_DOWN;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CMonitorWin::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_LEFTBTN;
		meb.mba = MouseBtnAct::MBA_UP;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CMonitorWin::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_RIGHTBTN;
		meb.mba = MouseBtnAct::MBA_DOUBLE;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnRButtonDblClk(nFlags, point);
}

void CMonitorWin::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_RIGHTBTN;
		meb.mba = MouseBtnAct::MBA_DOWN;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnRButtonDown(nFlags, point);
}

void CMonitorWin::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_RIGHTBTN;
		meb.mba = MouseBtnAct::MBA_UP;
		DealMouseEvent(point, meb);
	}

	CDialogEx::OnRButtonUp(nFlags, point);
}

void CMonitorWin::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_EnableWinCtrl) {
		MouseEventBody meb;
		meb.mb = MouseBtn::MB_NONE;
		meb.mba = MouseBtnAct::MBA_NONE;
		DealMouseEvent(point, meb); // 暂时注释，debug调试
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CMonitorWin::OnBnClickedButLockmachine()
{
	// TODO: 在此添加控件通知处理程序代码

	Packet* pPacket = new Packet;
	CPacketHandler::BuildPacketHeaderInPacket(*pPacket, StatusCode::SC_NONE,
		ChecksumType::CT_NONE, 0, ReqRes::RR_REQUEST | CommandType::CMD_LOCK_MACHINE);
	CClientController::ReqInfo rInfo(pPacket, false);
	CClientController::GetInstance()->SendRequest(rInfo);
}

void CMonitorWin::OnBnClickedButUnlockmachine()
{
	// TODO: 在此添加控件通知处理程序代码

	Packet* pPacket = new Packet;
	CPacketHandler::BuildPacketHeaderInPacket(*pPacket, StatusCode::SC_NONE,
		ChecksumType::CT_NONE, 0, ReqRes::RR_REQUEST | CommandType::CMD_UNLOCK_MACHINE);
	CClientController::ReqInfo rInfo(pPacket, false);
	CClientController::GetInstance()->SendRequest(rInfo);
}
