// DlProgDlg.cpp: 实现文件
//

#include "pch.h"
#include "DlProgDlg.h"
#include "afxdialogex.h"
#include "Scoped.h"
#include "Packet.h"
#include "ClientController.h"


// CDlProgDlg 对话框

IMPLEMENT_DYNAMIC(CDlProgDlg, CDialog)

CDlProgDlg::CDlProgDlg(CWnd* pParent /*=nullptr*/) :
	CDialog(IDD_DOWNLOAD_FILE, pParent),
	m_isDownloading(false),
	m_fileID(0),
	m_offset(0),
	m_totalSize(0),
	m_StopDownloading(true),
	m_EntryDownloadFileThreadID(0),
	m_EntryDownloadFileThread(NULL)
{}

CDlProgDlg::~CDlProgDlg()
{
	if (m_EntryDownloadFileThread) {
		WaitForSingleObject(m_EntryDownloadFileThread, 100);
		CloseHandle(m_EntryDownloadFileThread);
		m_EntryDownloadFileThread = NULL;
	}
}

void CDlProgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROG_DWONLOAD_FILE, m_prog_dlfile);
}


BEGIN_MESSAGE_MAP(CDlProgDlg, CDialog)
	ON_MESSAGE(WM_USER_STARTDOWNLOAD, &CDlProgDlg::OnStartDownload)
	ON_MESSAGE(WM_USER_STOPDOWNLOAD, &CDlProgDlg::OnStopDownload)
	ON_MESSAGE(WM_USER_DOWNLOADPROG, &CDlProgDlg::OnDownloadProg)
	ON_MESSAGE(WM_USER_DOWNLOADFINISH, &CDlProgDlg::OnDownloadFinish)
	ON_MESSAGE(WM_USER_DOWNLOAD_ERR, &CDlProgDlg::OnDownloadErr)
END_MESSAGE_MAP()


// CDlProgDlg 消息处理程序

void CDlProgDlg::ResetProg()
{
	m_prog_dlfile.SetRange(0, 100);
	m_prog_dlfile.SetStep(1);
	m_prog_dlfile.SetPos(0);
}

void CDlProgDlg::ClearDownloadFileInfo()
{
	m_fileID = 0;
	m_totalSize = 0;
	m_offset = 0;
	m_downloadFileName.Empty();
	m_saveFilePath.Empty();
}

void CDlProgDlg::StartDownload(uint32_t fileID, uint64_t totalSize, CString downloadFileName, CString savefilePath, bool force)
{
	if (m_isDownloading && !force) {
		return;
	}
	m_fileID = fileID;
	m_offset = 0;
	m_totalSize = totalSize;
	m_downloadFileName = downloadFileName;
	m_saveFilePath = savefilePath;
	ResetProg();
	PostMessage(WM_USER_STARTDOWNLOAD, 0, 0);
}

void CDlProgDlg::StopDownload()
{
	if (m_isDownloading) {
		PostMessage(WM_USER_STOPDOWNLOAD, 0, 0);
	}
}

afx_msg LRESULT CDlProgDlg::OnStartDownload(WPARAM wParam, LPARAM lParam)
{
	if (!StopDownloadFileThread()) {
		return -1;
	}

	m_StopDownloading = false;
	m_EntryDownloadFileThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL,
						0, EntryDownloadFileThread, this, 0, &m_EntryDownloadFileThreadID));
	return 0;
}

afx_msg LRESULT CDlProgDlg::OnStopDownload(WPARAM wParam, LPARAM lParam)
{
	if (StopDownloadFileThread()) {
		return 0;
	}
	return -1;
}

afx_msg LRESULT CDlProgDlg::OnDownloadProg(WPARAM wParam, LPARAM lParam)
{
	double offset = m_offset;
	double ratio = offset / static_cast<double>(m_totalSize);

	m_prog_dlfile.SetPos(ratio * 100);
	return 0;
}

afx_msg LRESULT CDlProgDlg::OnDownloadFinish(WPARAM wParam, LPARAM lParam)
{
	m_prog_dlfile.SetPos(100);
	MessageBox(_T("下载成功"), _T("文件下载"), MB_OK | MB_ICONINFORMATION);
	ShowWindow(SW_HIDE);
	return 0;
}

afx_msg LRESULT CDlProgDlg::OnDownloadErr(WPARAM wParam, LPARAM lParam)
{
	MessageBox(_T("下载失败"), _T("文件下载"), MB_OK | MB_ICONERROR);
	ShowWindow(SW_HIDE);
	return 0;
}

bool CDlProgDlg::StopDownloadFileThread()
{
	if (m_isDownloading) {
		m_StopDownloading = true;
		DWORD ret = WaitForSingleObject(m_EntryDownloadFileThread, 3000);
		if (ret == WAIT_TIMEOUT) {
			TRACE("%s : %s\n", __FUNCTION__, _T("Thread no exit!!"));
			return false;
		}
	}

	if (m_EntryDownloadFileThread) {
		CloseHandle(m_EntryDownloadFileThread);
		m_EntryDownloadFileThread = NULL;
	}

	m_StopDownloading = true;
	return true;
}

unsigned int __stdcall CDlProgDlg::EntryDownloadFileThread(void* arg)
{
	CDlProgDlg* thiz = reinterpret_cast<CDlProgDlg*>(arg);
	thiz->DownloadFile();
	return 0;
}

void CDlProgDlg::DownloadFile()
{
	m_isDownloading = true;
	FILE* pf = fopen(m_saveFilePath, "wb+");
	if (pf == NULL) {
		PostMessage(WM_USER_DOWNLOAD_ERR, 0, 0);
		m_isDownloading = false;
		return;
	}
	Scoped<decltype(pf), decltype(&fclose)> scopPF(pf, fclose);
	Packet inPacket, outPacket;
	DWORD handleID = ::GetCurrentThreadId();
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Scoped<HANDLE, decltype(&CloseHandle)> scoped(hEvent, &CloseHandle);
	Packet* pOutPacket = &outPacket; Packet** ppOutPacket = &pOutPacket;
	ReqFileBody rfBody;
	rfBody.fileID = m_fileID;
	rfBody.ctrlCode = ReqFileBody::RFB_CC_NEXT;
	ReqFileBody::Serialize(rfBody, inPacket.body);
	CPacketHandler::BuildPacketHeaderInPacket(inPacket, StatusCode::SC_NONE, ChecksumType::CT_SUM,
				handleID, ReqRes::RR_REQUEST | CommandType::CMD_DOWNLOAD_FILE);
	FileBody fBody;
	while (!m_StopDownloading) {
		if (CClientController::GetInstance()->RegisterResponse(handleID, ppOutPacket, hEvent) < 0) {
			PostMessage(WM_USER_DOWNLOAD_ERR, 0, 0);
			break;
		}

		if (CClientController::GetInstance()->SendRequest(&inPacket) < 0) {
			PostMessage(WM_USER_DOWNLOAD_ERR, 0, 0);
			break;
		}

		WaitForSingleObject(hEvent, INFINITE);
		ResetEvent(hEvent);

		if (
			((*ppOutPacket) == nullptr) ||
			(!CPacketHandler::ValidatePacket(outPacket, ReqRes::RR_RESPONSE | CommandType::CMD_DOWNLOAD_FILE)) ||
			(!FileBody::Deserialize(outPacket.body, fBody))
			)
		{
			PostMessage(WM_USER_DOWNLOAD_ERR, 0, 0);
			break;
		}
		
		fwrite(fBody.chunk.GetReadPtr(), sizeof(char), fBody.chunkSize, pf);
		m_offset += fBody.chunkSize;
		if (m_offset == m_totalSize || fBody.isLastChunk == 1) {
			if (m_offset != m_totalSize || fBody.isLastChunk != 1) {
				PostMessage(WM_USER_DOWNLOAD_ERR, 0, 0);
				break;
			}
			PostMessage(WM_USER_DOWNLOADFINISH, 0, 0);
			break;
		}

		fBody.Clear();
		outPacket.Clear();
		PostMessage(WM_USER_DOWNLOADPROG, 0, 0);
	}

	m_isDownloading = false;
}
