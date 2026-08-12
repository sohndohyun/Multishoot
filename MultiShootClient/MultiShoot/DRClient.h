#pragma once

#include "DRObjectPool.h"
#include "DRQueue.h"
#include "RingBuffer.h"
#include "DRPacket.h"
#include <WinSock2.h>
#include <atomic>

class DRClient
{
private:
	enum class IOtype
	{
		SEND, RECV, DCON, SHUTDOWN
	};

	typedef struct
	{
		OVERLAPPED overlapped;
		char buffer[BUFSIZ];
		WSABUF wsabuf;
		IOtype rwMode;
		int totalBytes;
	}PER_IO_INFO, * LPPER_IO_INFO;

	typedef struct
	{
		IOtype type;
		int size;
		DRPacket* packet;
	}PER_PROCESS_INFO;

public:
	DRClient();
	virtual ~DRClient();

	int init(const char* ip, int port);
	bool start();
	void send_data(char* data, int size);
	void wait();
	void end();
	bool work() const { return running.load() && connected.load(); }

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

	bool RecvProcess(char* data, int size);
	bool PostRecv(LPPER_IO_INFO ioInfo);
	void Disconnect();
	void BeginIo();
	void CompleteIo();

	bool isInitialize;
	bool wsaStarted;
	int port;
	HANDLE iocp;
	SOCKET sock;
	SOCKADDR_IN addr;

	DRObjectPool<DRPacket> packetPool;
	DRObjectPool<PER_IO_INFO> ioPool;
	DRQueue<PER_PROCESS_INFO> recvQ;
	DRQueue<PER_PROCESS_INFO> sendQ;
	RingBuffer recvData;

	std::atomic<bool> running;
	std::atomic<bool> connected;
	std::atomic<bool> socketOpen;
	std::atomic<int> pendingIo;
	HANDLE allIoDone;
	HANDLE ioThread[2];
	HANDLE sendThread;
	HANDLE callbackThread;
};
