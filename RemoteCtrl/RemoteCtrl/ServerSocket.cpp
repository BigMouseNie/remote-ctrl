#include "pch.h"
#include "ServerSocket.h"

CServerSocket::CServerSocket()
	: m_sock(INVALID_SOCKET), m_clnt_sock(INVALID_SOCKET)
{
	if (!InitSockEnv()) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络!"),
			_T("初始化失败"), MB_OK | MB_ICONERROR);
		exit(0);
	}
}	

CServerSocket::~CServerSocket()
{
	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
	}
	if (m_clnt_sock != INVALID_SOCKET) {
		closesocket(m_clnt_sock);
	}
	WSACleanup();
}

BOOL CServerSocket::InitSockEnv()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}

	return TRUE;
}

BOOL CServerSocket::InitSocket(uint16_t port)
{
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	// TODO: 校验
	if (m_sock == INVALID_SOCKET) {
		return FALSE;
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	int res = bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (res == -1) {
		closesocket(m_sock);
		return FALSE;
	}

	res = listen(m_sock, 1);
	if (res == -1) {
		closesocket(m_sock);
		return FALSE;	
	}

	return TRUE;
}

int CServerSocket::Run(DealPacketCallBack dpcb, uint16_t port)
{
	if (!InitSocket(port)) {
		MessageBox(NULL, _T("初始化套接字失败!"),
			_T("初始化失败"), MB_OK | MB_ICONERROR);
		return -1;
	}

	Packet recvPacket;
	Packet sendPacket;
	while (TRUE) {
		if (FALSE == AcceptClint()) {
			TRACE("%s : %s\n", __FUNCTION__, _T("accept client failed!"));
			continue;
		}
		int count = 0;
		while (TRUE) {
			recvPacket.Clear();
			sendPacket.Clear();
			int readRes = ReadPacket(recvPacket);
			if (readRes > 0) {
				count = 0;
				Packet* pOutPacket = &sendPacket;
				if (dpcb(recvPacket, &pOutPacket) >= 0) {
					if (!pOutPacket) {
						continue;
					}
					int sendRes = SendPacket(sendPacket);
					if (sendRes <= 0) {
						TRACE("%s : %s\n", __FUNCTION__, _T("send packet failed!"));
						break;
					}
				}
				else {
					TRACE("%s : %s\n", __FUNCTION__, _T("deal packet failed!"));
					break;
				}
			}
			else if (readRes < 0) {
				TRACE("%s : %s\n", __FUNCTION__, _T("Client close or error!"));
				break;
			}
			else {
				TRACE("%s : %s\n", __FUNCTION__, _T("Incomplete package!"));
				if (count++ > 64) {
					TRACE("%s : %s\n", __FUNCTION__, _T("Incomplete package(Retried 64 times break)!"));
					break;
				}
				continue;
			}
		}
		GetInstance()->CloseClient();
	}
	return 0;
}

BOOL CServerSocket::AcceptClint()
{
	sockaddr_in clnt_addr;
	int clnt_addr_len = sizeof(sockaddr_in);
	m_clnt_sock = accept(m_sock, (sockaddr*)&clnt_addr, &clnt_addr_len);
	
	if (m_clnt_sock == INVALID_SOCKET) {
		return FALSE;
	}

	m_hpacket.SetSocket(m_clnt_sock);
	return TRUE;
}

void CServerSocket::CloseClient()
{
	if (m_clnt_sock != INVALID_SOCKET) {
		closesocket(m_clnt_sock);
	}
}

int CServerSocket::SendPacket(const PacketHeader& header, const char* body)
{
	return m_hpacket.SendPacket(&header, body);
}

int CServerSocket::SendPacket(const Packet& packet)
{
	return m_hpacket.SendPacket(packet);
}

int CServerSocket::ReadPacket(Packet& packet)
{
	return m_hpacket.GetPacket(packet);
}
