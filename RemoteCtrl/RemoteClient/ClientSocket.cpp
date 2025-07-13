#include "pch.h"
#include "ClientSocket.h"

CClientSocket::CClientSocket()
	: m_sock(INVALID_SOCKET)
{
	if (!InitSockEnv()) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络!"),
			_T("初始化失败"), MB_OK | MB_ICONERROR);
		exit(0);
	}
}

CClientSocket::~CClientSocket()
{
	if (m_sock != INVALID_SOCKET) {
		
		closesocket(m_sock);
	}
	WSACleanup();
	TRACE("delete CClientSocket\n");
}

BOOL CClientSocket::InitSockEnv()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}

	return TRUE;
}

BOOL CClientSocket::Connect(DWORD ip, DWORD port)
{
	Disconnect();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	// TODO: 校验
	if (m_sock == INVALID_SOCKET) {
		return FALSE;
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(ip);
	serv_addr.sin_port = htons(port);

	if (SOCKET_ERROR == connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
		return FALSE;
	}

	m_hpacket.SetSocket(m_sock);
	return TRUE;
}

void CClientSocket::Disconnect()
{
	if (m_sock != INVALID_SOCKET) {
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
}

int CClientSocket::SendPacket(const PacketHeader& header, const char* body)
{
	return m_hpacket.SendPacket(header, body);
}

int CClientSocket::SendPacket(const Packet& packet)
{
	return m_hpacket.SendPacket(packet);
}

int CClientSocket::SafeSendPacket(const Packet& packet)
{
	return m_hpacket.SafeSendPacket(packet);
}

int CClientSocket::ReadPacket(Packet& packet)
{
	while (TRUE) {
		int res = m_hpacket.GetPacket(packet);
		if (res == 0) {
			Sleep(1);
			continue;
		}
		return res;
	}
}

ScopedConnection::ScopedConnection(DWORD ip, DWORD port)
	: conn(nullptr)
{
	if (CClientSocket::GetInstance()->Connect(ip, port)) {
		conn = CClientSocket::GetInstance();
	}
}

ScopedConnection::~ScopedConnection()
{
	if (conn) {
		conn->Disconnect();
	}
}

CClientSocket* ScopedConnection::GetConn()
{
	return conn;
}
