#include "pch.h"
#include "CmdHandler.h"
#include "Scoped.h"
#include "resource.h"

#include <direct.h>
#include <atlimage.h>
#include <objidl.h>   // 依赖项
#include <gdiplus.h>

ChecksumType CCmdHandler::DefCsType = ChecksumType::CT_SUM;
bool CCmdHandler::IsLockedMachine = false;
CLockDialog CCmdHandler::lockDlg;
unsigned int CCmdHandler::lockDlgTID;
const std::unordered_map<CommandType, DealPacketCallBack> CCmdHandler::IdxCmdHandler = {
	{ CommandType::CMD_TEST,				&TEST_handler },
	{ CommandType::CMD_DISK_PART,			&DISK_PART_handler },
	{ CommandType::CMD_LIST_FILE,			&LIST_FILE_handler },
	{ CommandType::CMD_RUN_FILE,			&RUN_FILE_handler },
	{ CommandType::CMD_DEL_FILE,			&DEL_FILE_handler },
	{ CommandType::CMD_DOWNLOAD_FILE,		&DOWNLOAD_FILE_handler },
	{ CommandType::CMD_SEND_SCREEN,			&SEND_SCREEN_handler },
	{ CommandType::CMD_MOUSE_EVENT,			&MOUSE_EVENT_handler },
	{ CommandType::CMD_LOCK_MACHINE,		&LOCK_MACHINE_handler },
	{ CommandType::CMD_UNLOCK_MACHINE,		&UNLOCK_MACHINE_handler },
	{ CommandType::CMD_INVALID_VALUE,		NULL }
};
std::atomic<bool> CCmdHandler::isRelese = false;

std::condition_variable CCmdHandler::NoWait;
const size_t CCmdHandler::MaxidleSec = 30;	// 调试时尽量大
std::mutex CCmdHandler::FDSMapMtx;
std::map<uint32_t, FileDownloadSession*> CCmdHandler::FDSMap;
HANDLE CCmdHandler::FDSRecycleThreadHANDLE = nullptr;

ULONG_PTR CCmdHandler::g_gdiplusToken = 0;

bool CCmdHandler::Init()
{
	static bool isInited = false;
	if (isInited) {
		return false;
	}
	isInited = true;
	FDSRecycleThreadHANDLE = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, FDSRecycleThread, NULL, 0, 0));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

	return FDSRecycleThreadHANDLE != nullptr;
}

void CCmdHandler::Release()
{
	isRelese = true;
	NoWait.notify_all();
	if (FDSRecycleThreadHANDLE) {
		WaitForSingleObject(FDSRecycleThreadHANDLE, 100);
		CloseHandle(FDSRecycleThreadHANDLE);
		FDSRecycleThreadHANDLE = NULL;
	}

	std::lock_guard<std::mutex> lock(FDSMapMtx);
	for (auto elem : FDSMap) {
		FileDownloadSession* fds = elem.second;
		delete fds;
		fds = nullptr;
	}
	FDSMap.clear();

	Gdiplus::GdiplusShutdown(g_gdiplusToken);
}

bool CCmdHandler::SetChecksumType(ChecksumType csType)
{
	if (csType < ChecksumType::CT_INVALID_VALUE) {
		DefCsType = csType;
		return true;
	}
	return false;
}

ChecksumType CCmdHandler::GetChecksumType()
{
	return DefCsType;
}

int CCmdHandler::DealPacket(const Packet& inPacket, Packet** ppOutPacket)
{
    CommandType cmd = static_cast<CommandType>(inPacket.header.messageType & EXTRACTIONCMD);
	if (cmd >= CommandType::CMD_INVALID_VALUE) {
		return -1;
	}

	if (!CPacketHandler::ValidatePacket(inPacket, 0, false)) {
		return -1;
	}

	auto it = IdxCmdHandler.find(cmd);
	if (it != IdxCmdHandler.end() && it->second) {
		return it->second(inPacket, ppOutPacket);
	}
	return -1;
}

int CCmdHandler::TEST_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	if (inPacket.body.TryPeekExact(outPacket.body, inPacket.header.bodyLength)) {
		CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
			ReqRes::RR_RESPONSE | CommandType::CMD_TEST, StatusCode::SC_OK);
	}
	else {
		outPacket.body.Clear();
		CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_TEST, StatusCode::SC_ERR_PACKET);
	}
	return 0;
}

int CCmdHandler::DISK_PART_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	uint32_t diskPart = 0;
	uint32_t flag = 1;
	for (int i = 1; i < 26; ++i) {
		if (_chdrive(i) == 0) {
			diskPart |= flag;
		}
		flag <<= 1;
	}

	outPacket.body.Write(reinterpret_cast<char*>(&diskPart), sizeof(uint32_t) / sizeof(char));
	CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
        ReqRes::RR_RESPONSE | CommandType::CMD_DISK_PART, StatusCode::SC_OK);
	return 0;
}

int CCmdHandler::LIST_FILE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	Buffer fPath;
	if (!inPacket.body.TryPeekExact(fPath, inPacket.header.bodyLength)) {
		outPacket.body.Clear();
		CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_LIST_FILE, StatusCode::SC_ERR_PACKET);
		return 0;
	}
	fPath.Write("\\*\0", 4);

	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(fPath.GetReadPtr(), &findData);
	Scoped<decltype(hFind), decltype(&FindClose)> scopedHFind(hFind, &FindClose);
	if (hFind == INVALID_HANDLE_VALUE) {
		outPacket.body.Clear();
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_LIST_FILE, StatusCode::SC_ERR_NOTFOUND);
		return 0;
	}

	FileListBody flBody;
	char ftVal;
	FileNameLen flenVal;
	size_t sendTotalBytes = 0;
	do {
		// 忽略 . 和 .. 以及系统文件
		if (_tcscmp(findData.cFileName, _T(".")) == 0 ||
			_tcscmp(findData.cFileName, _T("..")) == 0 ||
			(findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
		{
			continue;
		}

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ftVal = FileType::FT_DIR;
			flBody.fileTypes.Write(&ftVal, sizeof(FileType));
		}
		else {
			ftVal = FileType::FT_FILE;
			flBody.fileTypes.Write(&ftVal, sizeof(FileType));
		}

		flenVal = strlen(findData.cFileName);
		flBody.fileNameLens.Write(reinterpret_cast<char*>(&flenVal), sizeof(FileNameLen));
		flBody.fileNameBuffer.Write(findData.cFileName, flenVal);
		flBody.fileCnt++;
	} while (FindNextFile(hFind, &findData));

	if (FileListBody::Serialize(flBody, outPacket.body)) {
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_LIST_FILE, StatusCode::SC_OK);
	}
	else {
		outPacket.body.Clear();
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_LIST_FILE, StatusCode::SC_ERR_INTERNAL);
	}
	return 0;
}

unsigned int __stdcall CCmdHandler::FDSRecycleThread(void* arg)
{
	while (!isRelese) {
		std::unique_lock<std::mutex> uniLock(FDSMapMtx);
		NoWait.wait_for(uniLock, std::chrono::seconds(MaxidleSec));
		if (isRelese) { return 0; }
		time_t now = time(nullptr);
		for (auto it = FDSMap.begin(); it != FDSMap.end(); ) {
			if (now - it->second->GetLastActiveTime() > MaxidleSec) {
				FileDownloadSession* fds = it->second;
				delete fds; fds = nullptr;
				it = FDSMap.erase(it); // erase 会返回下一个有效迭代器
			}
			else {
				++it;
			}
		}
	}
	return 0;
}

void CCmdHandler::DOWNLOAD_FILE_BEGIN_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode)
{
	FileDownloadSession* fds = new FileDownloadSession;
	FileBody fBody;
	bool result = false;
	if (fds->OpenFile(rfBody.filePath.GetReadPtr())) {
		fds->GetBaseFileInfo(fBody);
		FileBody::Serialize(fBody, outBody);
		std::lock_guard<std::mutex> lock(FDSMapMtx);
		auto insertRes = FDSMap.insert({ fds->GetFileID(), fds });
		if (insertRes.second) {
			scode = StatusCode::SC_OK;
			return;
		}
	}

	scode = StatusCode::SC_ERR_NOTFOUND;
	delete fds;
	return;
}

void CCmdHandler::DOWNLOAD_FILE_STOP_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode)
{
	std::lock_guard<std::mutex> lock(FDSMapMtx);
	auto it = FDSMap.find(rfBody.fileID);
	if (it == FDSMap.end()) {
		scode = StatusCode::SC_ERR;
		return;
	}
	scode = StatusCode::SC_OK;
}

void CCmdHandler::DOWNLOAD_FILE_NEXT_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode)
{
	std::lock_guard<std::mutex> lock(FDSMapMtx);
	auto it = FDSMap.find(rfBody.fileID);
	if (it == FDSMap.end()) {
		scode = StatusCode::SC_ERR;
		return;
	}
	FileDownloadSession* fds = it->second;
	if (fds->IsFinished()) {
		scode = StatusCode::SC_ERR;
		return;
	}

	if (fds->GetFileBodyToBuffer(outBody)) {
		scode = StatusCode::SC_OK;
	}
	else {
		scode = StatusCode::SC_ERR_INTERNAL;
	}
}

void CCmdHandler::DOWNLOAD_FILE_PREV_handler(const ReqFileBody& rfBody, Buffer& outBody, StatusCode& scode)
{
	std::lock_guard<std::mutex> lock(FDSMapMtx);
	auto it = FDSMap.find(rfBody.fileID);
	if (it == FDSMap.end()) {
		scode = StatusCode::SC_ERR;
		return;
	}
	FileDownloadSession* fds = it->second;
	if (fds->GetFileBodyToBuffer(outBody, true)) {
		scode = StatusCode::SC_OK;
	}
	else {
		scode = StatusCode::SC_ERR_INTERNAL;
	}
}

int CCmdHandler::DOWNLOAD_FILE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	ReqFileBody rfBody;
	if (!ReqFileBody::Deserialize(inPacket.body, rfBody)) {
		CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_DOWNLOAD_FILE, StatusCode::SC_ERR_PACKET);
		return 0;
	}
	FileBody fBody;
	StatusCode scode;
	switch (rfBody.ctrlCode)
	{
	case ReqFileBody::RFB_CC_BEGIN:
		rfBody.filePath.Write("\0", 1);
		DOWNLOAD_FILE_BEGIN_handler(rfBody, outPacket.body, scode);
		break;

	case ReqFileBody::RFB_CC_STOP:
		DOWNLOAD_FILE_STOP_handler(rfBody, outPacket.body, scode);
		break;

	case ReqFileBody::RFB_CC_NEXT:
		DOWNLOAD_FILE_NEXT_handler(rfBody, outPacket.body, scode);
		break;

	case ReqFileBody::RFB_CC_PREV:
		DOWNLOAD_FILE_PREV_handler(rfBody, outPacket.body, scode);
		break;

	default:
		scode = StatusCode::SC_ERR_PACKET;
		break;
	}

    CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
        ReqRes::RR_RESPONSE | CommandType::CMD_DOWNLOAD_FILE, scode);
	return 0;
}

int CCmdHandler::RUN_FILE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	Buffer fPath;
	if (!inPacket.body.TryPeekExact(fPath, inPacket.header.bodyLength)) {
		outPacket.body.Clear();
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_RUN_FILE, StatusCode::SC_ERR_PACKET);
		return 0;
	}
	fPath.Write("\0", 1);

	INT_PTR res = (INT_PTR)ShellExecute(NULL, NULL, fPath.GetReadPtr(), NULL, NULL, SW_SHOWNORMAL);
	if (res <= 32) {
		TRACE("%s : error code : %d\n", __FUNCTION__, res);
	}
    CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
        ReqRes::RR_RESPONSE | CommandType::CMD_RUN_FILE, StatusCode::SC_OK);
	return 0;
}

int CCmdHandler::DEL_FILE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;
	Buffer fPath;
	if (!inPacket.body.TryPeekExact(fPath, inPacket.header.bodyLength)) {
		outPacket.body.Clear();
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
            ReqRes::RR_RESPONSE | CommandType::CMD_DEL_FILE, StatusCode::SC_ERR_PACKET);
		return 0;
	}
	fPath.Write("\0", 1);

	int res = DeleteFile(fPath.GetReadPtr());
	if (res == 0) {
		TRACE("%s : error code : %ld\n", __FUNCTION__, GetLastError());
	}
    CPacketHandler::BuildPacketHeaderInPacket(outPacket, DefCsType,
        ReqRes::RR_RESPONSE | CommandType::CMD_DEL_FILE, StatusCode::SC_OK);
	return 0;
}

int CCmdHandler::SEND_SCREEN_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	Packet& outPacket = **ppOutPacket;
    outPacket.header.handleID = inPacket.header.handleID;

	CImage screen;
	HDC hScreen = ::GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);

	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
	::ReleaseDC(NULL, hScreen);
	screen.ReleaseDC();

	IStream* pStream = SHCreateMemStream(nullptr, 0);
	if (!pStream) {
		outPacket.body.Clear();
        CPacketHandler::BuildPacketHeaderInPacket(outPacket, ChecksumType::CT_NONE,
            ReqRes::RR_RESPONSE | CommandType::CMD_SEND_SCREEN, StatusCode::SC_ERR_INTERNAL);
		return false;
	}

	int result = 0;
	if (SUCCEEDED(screen.Save(pStream, Gdiplus::ImageFormatJPEG))) {
		STATSTG stat;
		pStream->Stat(&stat, STATFLAG_NONAME);
		ULONG size = (ULONG)stat.cbSize.QuadPart; // 获取内存流的大小，即JPEG数据长度

		LARGE_INTEGER liZero = {};
		pStream->Seek(liZero, STREAM_SEEK_SET, NULL); // 指针重置到流开头

		ULONG bytesRead = 0;
		RawBuffer rawBuf(outPacket.body, size);
		char* rawWPtr = rawBuf.GetRawWritePtr();
		pStream->Read(rawWPtr, size, &bytesRead);
		rawBuf.Written(bytesRead);
	}

    CPacketHandler::BuildPacketHeaderInPacket(outPacket, ChecksumType::CT_NONE,
        ReqRes::RR_RESPONSE | CommandType::CMD_SEND_SCREEN, StatusCode::SC_OK);

	return 0;
}

int CCmdHandler::MOUSE_EVENT_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	*ppOutPacket = nullptr;	// no reply
	MouseEventBody meBody;
	if (!MouseEventBody::Deserialize(inPacket.body, meBody)) {
		return 0;
	}

	SimulateMouseClick(meBody);
	return 0;
}

void CCmdHandler::SimulateMouseClick(const MouseEventBody& meb)
{
	WHXY curWinWH;
	curWinWH.WX = GetSystemMetrics(SM_CXSCREEN);  // 屏幕宽度
	curWinWH.HY = GetSystemMetrics(SM_CYSCREEN); // 屏幕高度

	WHXY curPos = meb.GetCurWinXY(curWinWH);

	SetCursorPos(curPos.WX, curPos.HY);

	switch (meb.mb)
	{
	case MB_LEFTBTN:
		switch (meb.mba)
		{
		case MBA_DOWN:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			break;
		case MBA_UP:
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case MBA_DOUBLE:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		default:
			break;
		}
		break;

	case MB_RIGHTBTN:
		switch (meb.mba)
		{
		case MBA_DOWN:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			break;
		case MBA_UP:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case MBA_DOUBLE:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		default:
			break;
		}
		break;

	case MB_MIDBTN:
		switch (meb.mba)
		{
		case MBA_DOWN:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			break;
		case MBA_UP:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			break;
		case MBA_DOUBLE:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

unsigned __stdcall CCmdHandler::ThreadLockMachine(void* arg)
{
	TRACE("func:%s, tid:%d", __FUNCTION__, GetCurrentThreadId());
	lockDlg.Create(IDD_DIALOG_LOCK_INFO, NULL);

	CRect rect;
	rect.left = 0;
	rect.top = 0;
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN); // 屏幕宽度
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN); // 屏幕高度
	rect.right = screenWidth;
	rect.bottom = screenHeight;

	TRACE("%d, %d", rect.right, rect.bottom);

	ShowCursor(FALSE);
	lockDlg.ShowWindow(SW_SHOW);
	lockDlg.MoveWindow(rect);
	lockDlg.SetWindowPos(&lockDlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

	lockDlg.GetWindowRect(&rect);
	ClipCursor(rect);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			if (msg.wParam == 0x41) {   // 'A'键按下退出循环锁机结束
				break;
			}
		}
	}
	lockDlg.DestroyWindow();

	ClipCursor(rect);
	ShowCursor(TRUE);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
	_endthreadex(0);
	IsLockedMachine = false;
	return 0;
}

int CCmdHandler::LOCK_MACHINE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	*ppOutPacket = nullptr;	// no reply
	if ((lockDlg.m_hWnd != NULL) && lockDlg.m_hWnd != INVALID_HANDLE_VALUE) {
		return 0;
    }

	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadLockMachine, NULL, 0, &lockDlgTID);
	if (hThread != NULL) {
		IsLockedMachine = true;
	}
	else {
		IsLockedMachine = false;
	}
    return 0;
}

int CCmdHandler::UNLOCK_MACHINE_handler(const Packet& inPacket, Packet** ppOutPacket)
{
	*ppOutPacket = nullptr;	// no reply
	if (IsLockedMachine) {
		PostThreadMessage(lockDlgTID, WM_KEYDOWN, 0x41, 0); // 将'A'键按下消息发送到消息队列
	}
	return 0;
}

const uint32_t FileDownloadSession::minChunkSize = 128;
uint32_t FileDownloadSession::nextFileID = 1;
bool FileDownloadSession::OpenFile(const char* fPath)
{
	if (pf) { fclose(pf); pf = nullptr; }
    pf = fopen(fPath, "rb");
	if (pf == nullptr) {
		return false;
	}
	filePath.clear(); filePath = fPath;
	fileID = nextFileID++;
	lastOffset = 0;
	offset = 0;
	isFinished = false;
	lastActiveTime = time(nullptr);
	fseek(pf, 0, SEEK_END);
	totalSize = _ftelli64(pf);
	fseek(pf, 0, SEEK_SET);
	return true;
}

bool FileDownloadSession::GetBaseFileInfo(FileBody& fbBody) const
{
	if (!pf) { return false; }
	fbBody.fileID = fileID;
	fbBody.totalSize = totalSize;
	return true;
}

bool FileDownloadSession::GetFileBodyToBuffer(Buffer& fBodyBuf, bool getPreChunk)
{
	if (!pf) { return false; }
	RawBuffer rawBuf(fBodyBuf, chunkSize + FileBody::minCompactSize);
	char* rawWPtr = rawBuf.GetRawWritePtr();
	size_t writtenLen = 0;
	if (getPreChunk) {
		writtenLen = ReadPreChunk(rawWPtr + FileBody::minCompactSize);
	}
	else {
		writtenLen = ReadChunk(rawWPtr + FileBody::minCompactSize);
	}

	FileBody fBody;
	fBody.fileID = fileID;
	fBody.isLastChunk = (isFinished) ? 1 : 0;
	fBody.offset = offset;
	fBody.chunkSize = writtenLen;
	fBody.totalSize = totalSize;
	FileBody::SerializeNoSafe(fBody, rawWPtr);
	rawBuf.Written(writtenLen + FileBody::minCompactSize);
	lastActiveTime = time(nullptr);
	return true;
}

size_t FileDownloadSession::ReadChunk(char* outChunk)
{
	lastOffset = offset;
	size_t readLen = fread(outChunk, sizeof(char), chunkSize, pf);
	offset += readLen;
	isFinished = (offset == totalSize);
	return readLen;
}

void FileDownloadSession::ReadChunk(Buffer& outChunk)
{
	RawBuffer rawBuf(outChunk, chunkSize);
	char* rawWPtr = rawBuf.GetRawWritePtr();
	rawBuf.Written(ReadChunk(rawWPtr));
}

size_t FileDownloadSession::ReadPreChunk(char* outChunk)
{
	fseek(pf, lastOffset, SEEK_SET);
	size_t readLen = fread(outChunk, sizeof(char), chunkSize, pf);
	offset = lastOffset + readLen;
	isFinished = (offset == totalSize);
	return readLen;
}

void FileDownloadSession::ReadPreChunk(Buffer& outChunk)
{
	RawBuffer rawBuf(outChunk, chunkSize);
	char* rawWPtr = rawBuf.GetRawWritePtr();
	rawBuf.Written(ReadPreChunk(rawWPtr));
}

void FileDownloadSession::SetChunkSize(uint32_t cSize)
{
	chunkSize = (cSize < minChunkSize) ? minChunkSize : cSize;
}

uint32_t FileDownloadSession::GetFileID()
{
	return fileID;
}

bool FileDownloadSession::IsFinished()
{
	return isFinished;
}

time_t FileDownloadSession::GetLastActiveTime()
{
	return lastActiveTime;
}

void FileDownloadSession::Close()
{
	if (pf) {
		fclose(pf);
		pf = nullptr;
	}
	filePath.clear();
	fileID = 0;
	lastOffset = 0;
	offset = 0;
	isFinished = false;
}
