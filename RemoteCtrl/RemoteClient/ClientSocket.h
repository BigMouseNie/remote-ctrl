#pragma once

#include "Packet.h"
#include "Singleton.h"

class CClientSocket : public Singleton<CClientSocket>
{
	friend class Singleton<CClientSocket>;
public:
	int SendPacket(const PacketHeader& header, const char* body);
	int SendPacket(const Packet& packet);
	int SafeSendPacket(const Packet& packet);
	int ReadPacket(Packet& packet);
	BOOL Connect(DWORD ip, DWORD port);
	void Disconnect();

private:
	CClientSocket();
	~CClientSocket();

	BOOL InitSockEnv();

private:
	SOCKET m_sock;
	CPacketHandler m_hpacket;
};

class ScopedConnection
{
public:
	ScopedConnection(DWORD ip, DWORD port);
	~ScopedConnection();
	CClientSocket* GetConn();

private:
	CClientSocket* conn;
};
