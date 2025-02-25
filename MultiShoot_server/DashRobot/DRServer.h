#pragma once

class DRServer
{
private:
	enum class IOtype
	{
		SEND, RECV, LEAVE, ACCEPT
	};

	typedef struct
	{
		SOCKET hClientSock;
		SOCKADDR_IN clientAddr;
		RingBuffer recvData;
	}PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

	typedef struct
	{
		OVERLAPPED overlapped;
		WSABUF wsabuf;
		char buffer[BUFSIZ];
		IOtype rwMode; // READ or WRITE
	}PER_IO_DATA, * LPPER_IO_DATA;

	typedef struct
	{
		IOtype type;
		SOCKET sock;
		int size;
		DRPacket* packet;
	}PER_PROCESS_INFO;

public:
	DRServer();
	virtual ~DRServer() {}

	int init(int port, int threadCnt);
	void start();

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

	void RecvProcess(LPPER_HANDLE_DATA client, char* data, int size);

	// setting
	int port;
	HANDLE iocp;
	SOCKET svrSock;
	int threadCnt;

	//value
	bool isInitialized;

	//pool
	DRObjectPool<PER_IO_DATA> ioPool;
	DRObjectPool<DRPacket> packetPool;

	//queue
	DRQueue<PER_PROCESS_INFO> recvQ;
	DRQueue<PER_PROCESS_INFO> sendQ;
};

