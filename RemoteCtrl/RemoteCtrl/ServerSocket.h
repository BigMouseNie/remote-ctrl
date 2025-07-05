#pragma once

#include "Packet.h"
#include "CallBack.h"
#include "Singleton.h"

class CServerSocket : public Singleton<CServerSocket>
{
	friend class Singleton<CServerSocket>;
public:
	int Run(DealPacketCallBack dpcb, uint16_t port = 8081);
	BOOL AcceptClint();
	void CloseClient();
	int SendPacket(const PacketHeader& header, const char* body);
	int SendPacket(const Packet& packet);
	int ReadPacket(Packet& packet);

private:
	CServerSocket();
	~CServerSocket();

	BOOL InitSockEnv();
	BOOL InitSocket(uint16_t port);

private:
	SOCKET m_sock;
	SOCKET m_clnt_sock;
	CPacketHandler m_hpacket;
};
