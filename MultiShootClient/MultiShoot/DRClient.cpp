#include <WinSock2.h>
#include <stdio.h>
#include "DRClient.h"
#include <process.h>
#include "DRPacket.h"
#include <ws2tcpip.h>
#include <iostream>

DRClient::DRClient()
{
	iocp = 0;
	memset(&addr, 0, sizeof(addr));
	port = 0;
	sock = INVALID_SOCKET;


	isInitialize = false;
}

int DRClient::init(const char* ip, int port)
{
	if (isInitialize == true)
	{
		return 1;
	}


	WSADATA wsaData;

	this->port = port;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 2;

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
		return 3;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &(addr.sin_addr));
	addr.sin_port = htons(port);


	isInitialize = true;
	return 0;
}

bool DRClient::start()
{
	if (isInitialize == false)
		return false;

	_endisFalse = true;

	if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		return false;
	}
	OnConnected();

	HANDLE h = CreateIoCompletionPort((HANDLE)sock, iocp, sock, 0);
	ioThread[0] = (HANDLE)_beginthreadex(NULL, 0, IOThreadMain, (LPVOID)this, 0, NULL);
	ioThread[1] = (HANDLE)_beginthreadex(NULL, 0, IOThreadMain, (LPVOID)this, 0, NULL);
	sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, (LPVOID)this, 0, NULL);
	callbackThread = (HANDLE)_beginthreadex(NULL, 0, CallbackThread, (LPVOID)this, 0, NULL);

	
	DWORD flags = 0;
	LPPER_IO_INFO ioInfo = ioPool.Alloc();
	memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
	ioInfo->wsabuf.len = BUFSIZ;
	ioInfo->wsabuf.buf = ioInfo->buffer;
	ioInfo->rwMode = IOtype::RECV;
	WSARecv(sock, &ioInfo->wsabuf, 1, NULL, &flags, &ioInfo->overlapped, NULL);


	return true;
}

void DRClient::send_data(char* data, int size)
{
	auto packet = packetPool.Alloc();
	packet->init();
	packet->put(data, size);
	packet->header()->size = size;
	packet->header()->code = 12;

	sendQ.push({ IOtype::SEND, packet->size(), packet });
}

void DRClient::wait()
{
	WaitForSingleObject(callbackThread, INFINITE);
}

void DRClient::end() {
	_endisFalse = false;
	closesocket(sock);
	WSACleanup();
}

unsigned int _stdcall DRClient::IOThreadMain(void* clClass)
{
	DRClient* cl = (DRClient*)clClass;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_IO_INFO ioInfo;
	DWORD flags = 0;

	while (cl->_endisFalse)
	{
		GetQueuedCompletionStatus(cl->iocp, &bytesTrans, (PULONG_PTR)&sock, (LPOVERLAPPED*)&ioInfo, INFINITE);

		if (ioInfo == NULL)
		{
			//printf("%d\n", GetLastError());
			continue;
		}

		if (ioInfo->rwMode == IOtype::RECV)
		{
			if (bytesTrans == 0)
			{
				cl->ioPool.Dealloc(ioInfo);

				cl->recvQ.push({ IOtype::DCON, 0, NULL });

				closesocket(sock);
				continue;
			}
			else
			{
				cl->RecvProcess(ioInfo->buffer, bytesTrans);

				memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
				ioInfo->wsabuf.len = BUFSIZ;
				ioInfo->wsabuf.buf = ioInfo->buffer;
				ioInfo->rwMode = IOtype::RECV;
				WSARecv(sock, &ioInfo->wsabuf, 1, NULL, &flags, &ioInfo->overlapped, NULL);
			}
		}
		else
		{
			cl->recvQ.push({ IOtype::SEND, (int)bytesTrans, NULL });
			cl->ioPool.Dealloc(ioInfo);
		}
	}

	return 0;
}

unsigned int _stdcall DRClient::SendThread(void* clClass)
{
	//TODO
	auto cl = (DRClient*)clClass;
	auto sendq = &cl->sendQ;
	while (cl->_endisFalse)
	{
		PER_PROCESS_INFO info;
		if (!sendq->pop(&info))
			continue;

		auto ioInfo = cl->ioPool.Alloc();
		memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));

		auto packet = info.packet;
		ioInfo->wsabuf.len = packet->callPacket(ioInfo->buffer);
		ioInfo->wsabuf.buf = ioInfo->buffer;
		ioInfo->rwMode = IOtype::SEND;
		WSASend(cl->sock, &ioInfo->wsabuf, 1, NULL, 0, &ioInfo->overlapped, NULL);
	}
	return 0;
}

unsigned int _stdcall DRClient::CallbackThread(void* clClass)
{
	DRClient* cl = (DRClient*)clClass;
	while (cl->_endisFalse)
	{
		cl->OnUpdate();

		PER_PROCESS_INFO info;
		if (cl->recvQ.pop(&info))
		{
			switch (info.type)
			{
			case IOtype::SEND:
				cl->OnSend(info.size);
				break;
			case IOtype::RECV:
				cl->OnRecv(info.packet->getCallP(), info.size);
				cl->packetPool.Dealloc(info.packet);
				break;
			case IOtype::DCON:
				cl->OnDisconnected();
				break;
			}
		}
	}
	return 0;
}

void DRClient::RecvProcess(char* data, int size)
{
	int result;
	DRPacket::Header head;

	recvData.Lock();
	result = recvData.Put(data, size);
	if (result < size) {
		recvData.Unlock();
		return;
	}

	while (auto useSize = recvData.GetUseSize())
	{
		//check header;
		if (useSize < sizeof(DRPacket::Header)) {
			break;
		}
		recvData.Peek((char*)&head, sizeof(head));

		//check code
		//if(head.code != MYCODE)
		//  return;

		//check size;
		if (useSize < sizeof(head) + head.size) {
			break;
		}

		auto packet = packetPool.Alloc();
		packet->init();
		recvData.Get(packet->getBuf(), sizeof(head) + head.size);
		packet->movep(sizeof(head) + head.size);

		recvQ.push({ IOtype::RECV, packet->size(), packet });
	}
	recvData.Unlock();
}