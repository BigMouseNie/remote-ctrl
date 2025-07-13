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
    m_HEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CMonitorWin::~CMonitorWin()
{
	if (!m_cachedImage.IsNull()) {
		m_cachedImage.Destroy();
	}
    if (m_HEvent) {
        CloseHandle(m_HEvent);
        m_HEvent = NULL;
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
    ON_MESSAGE(WM_USER_FRAMEDATACOMING, &CMonitorWin::OnFrameDataComing)
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
    m_picture.GetClientRect(&m_pictureRect);
    TRACE("%s : %d,%d,%d,%d\n", __FUNCTION__, m_pictureRect.left, m_pictureRect.top, m_pictureRect.right, m_pictureRect.bottom);
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
        CDC* pDC = m_picture.GetDC();
        if (pDC) {
            pDC->SetStretchBltMode(HALFTONE);
            ::SetBrushOrgEx(pDC->GetSafeHdc(), 0, 0, nullptr);
            m_cachedImage.Draw(pDC->GetSafeHdc(), m_pictureRect);
            m_picture.ReleaseDC(pDC);
        }
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

    HANDLE handleID = this->GetSafeHwnd();
    MapPacket::ResourcePacket rp;
    if (!m_MapPacket.AppResourcePacket(WM_USER_FRAMEDATACOMING, rp)) {
        PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
        return;
    }

	CPacketHandler::BuildPacketHeaderInPacket(*(rp.inPacket), ChecksumType::CT_NONE,
		ReqRes::RR_REQUEST | CommandType::CMD_SEND_SCREEN, StatusCode::SC_NONE);
    CClientController::ReqInfo reqInfo(rp.inPacket, rp.outPacket, true, handleID, WM_USER_FRAMEDATACOMING);
    ResetEvent(m_HEvent);
	PostMessage(WM_USER_ENABLEWINCTRL, 0, 0);
	while (!m_StopMonitoring) {
        if (CClientController::GetInstance()->SendRequest(reqInfo) < 0) {
            PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
            break;
        }

		WaitForSingleObject(m_HEvent, INFINITE);
		ResetEvent(m_HEvent);
        if (m_StopMonitoring) {
            break;
        }

		if (
			(rp.outPacket->Empty()) ||
			(!CPacketHandler::ValidatePacket(*(rp.outPacket), ReqRes::RR_RESPONSE | CommandType::CMD_SEND_SCREEN))
		   )
		{
			PostMessage(WM_USER_REMOTEMONITOR_ERR, 0, 0);
			break;
		}

		DrawFrame(rp.outPacket->body);
        rp.outPacket->Clear();
		Sleep(200);
	}
	PostMessage(WM_USER_DISABLEWINCTRL, 0, 0);
    m_MapPacket.PutResourcePacket(WM_USER_FRAMEDATACOMING);
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
    SetEvent(m_HEvent);
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

afx_msg LRESULT CMonitorWin::OnFrameDataComing(WPARAM wParam, LPARAM lParam)
{
    SetEvent(m_HEvent);
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

	CPacketHandler::BuildPacketHeaderInPacket(*pPacket, ChecksumType::CT_NONE,
        ReqRes::RR_REQUEST | CommandType::CMD_MOUSE_EVENT, StatusCode::SC_NONE);

    CClientController::ReqInfo reqInfo(pPacket, nullptr, false, 0, 0, false);
	CClientController::GetInstance()->SendRequest(reqInfo);
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
    CPacketHandler::BuildPacketHeaderInPacket(*pPacket, ChecksumType::CT_NONE,
        ReqRes::RR_REQUEST | CommandType::CMD_LOCK_MACHINE, StatusCode::SC_NONE);
    CClientController::ReqInfo reqInfo(pPacket, nullptr, false, 0, 0, false);
    CClientController::GetInstance()->SendRequest(reqInfo);
}

void CMonitorWin::OnBnClickedButUnlockmachine()
{
	// TODO: 在此添加控件通知处理程序代码

    Packet* pPacket = new Packet;
    CPacketHandler::BuildPacketHeaderInPacket(*pPacket, ChecksumType::CT_NONE,
        ReqRes::RR_REQUEST | CommandType::CMD_UNLOCK_MACHINE, StatusCode::SC_NONE);
    CClientController::ReqInfo reqInfo(pPacket, nullptr, false, 0, 0, false);
    CClientController::GetInstance()->SendRequest(reqInfo);
}
