#pragma once

#include <atomic>
#include <unordered_map>
#include <vector>

class DRServer
{
private:
	enum class IOtype
	{
		SEND, RECV, LEAVE, ACCEPT, SHUTDOWN
	};

	typedef struct
	{
		SOCKET hClientSock;
		SOCKADDR_IN clientAddr;
		RingBuffer recvData;
		std::atomic<long> references;
		std::atomic<bool> closing;
	}PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

	typedef struct
	{
		OVERLAPPED overlapped;
		WSABUF wsabuf;
		char buffer[BUFSIZ];
		IOtype rwMode;
		int totalBytes;
	}PER_IO_DATA, * LPPER_IO_DATA;

	typedef struct
	{
		IOtype type;
		LPPER_HANDLE_DATA client;
		int size;
		DRPacket* packet;
	}PER_PROCESS_INFO;

public:
	DRServer();
	virtual ~DRServer();

	int init(int port, int threadCnt);
	void start();
	void end();

	void send_data(SOCKET sock, char* data, int size);

protected:
	virtual void OnUpdate(float dt) = 0;
	virtual void OnAccept(SOCKET sock) = 0;
	virtual void OnSend(SOCKET sock, int size) = 0;
	virtual void OnRecv(SOCKET sock, char* data, int size) = 0;
	virtual void OnLeave(SOCKET sock) = 0;

private:
	static unsigned int _stdcall AcceptThreadMain(void* svClass);
	static unsigned int _stdcall IOThreadMain(void* svClass);
	static unsigned int _stdcall SendThread(void* svClass);
	static unsigned int _stdcall CallbackThread(void* svClass);

	bool RecvProcess(LPPER_HANDLE_DATA client, char* data, int size);
	bool PostRecv(LPPER_HANDLE_DATA client, LPPER_IO_DATA ioInfo);
	void CloseClient(LPPER_HANDLE_DATA client, bool notify);
	void AddRef(LPPER_HANDLE_DATA client);
	void ReleaseClient(LPPER_HANDLE_DATA client);

	int port;
	HANDLE iocp;
	SOCKET svrSock;
	int threadCnt;

	bool isInitialized;
	bool wsaStarted;
	std::atomic<bool> running;
	std::atomic<int> activeClients;

	HANDLE acceptThread;
	HANDLE sendThread;
	HANDLE callbackThread;
	std::vector<HANDLE> ioThreads;

	DRObjectPool<PER_IO_DATA> ioPool;
	DRObjectPool<DRPacket> packetPool;

	DRQueue<PER_PROCESS_INFO> recvQ;
	DRQueue<PER_PROCESS_INFO> sendQ;
	std::unordered_map<SOCKET, LPPER_HANDLE_DATA> clients;
};
