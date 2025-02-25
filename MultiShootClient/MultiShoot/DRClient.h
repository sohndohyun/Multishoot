#pragma once

#include "DRObjectPool.h"
#include "DRQueue.h"
#include "RingBuffer.h"
#include "DRPacket.h"
#include <WinSock2.h>

class DRClient
{
private:
	enum class IOtype
	{
		SEND, RECV, DCON
	};

	typedef struct
	{
		OVERLAPPED overlapped;
		char buffer[BUFSIZ];
		WSABUF wsabuf;
		IOtype rwMode;
	}PER_IO_INFO,* LPPER_IO_INFO;

	typedef struct
	{
		IOtype type;
		int size;
		DRPacket* packet;
	}PER_PROCESS_INFO;
public:
	DRClient();
	virtual ~DRClient() {}

	int init(const char* ip, int port);
	bool start();

	void send_data(char* data, int size);

	void wait();

	void end();

protected:
	virtual void OnUpdate() = 0;
	virtual void OnConnected() = 0;
	virtual void OnSend(int size) = 0;
	virtual void OnRecv(char* data, int size) = 0;
	virtual void OnDisconnected() = 0;

private:
	static unsigned int _stdcall IOThreadMain(void* clClass);
	static unsigned int _stdcall SendThread(void* clClass);
	static unsigned int _stdcall CallbackThread(void* clClass);

	void RecvProcess(char* data, int size);


	bool isInitialize;

	int port;
	HANDLE iocp;

	SOCKET sock;
	SOCKADDR_IN addr;

	DRObjectPool<DRPacket> packetPool;
	DRObjectPool<PER_IO_INFO> ioPool;

	DRQueue<PER_PROCESS_INFO> recvQ;
	DRQueue<PER_PROCESS_INFO> sendQ;

	RingBuffer recvData;

	bool _endisFalse;
	HANDLE ioThread[2];
	HANDLE sendThread;
	HANDLE callbackThread;
};

