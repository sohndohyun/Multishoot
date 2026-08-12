#include <WinSock2.h>
#include <stdio.h>
#include "DRClient.h"
#include <process.h>
#include <ws2tcpip.h>

DRClient::DRClient()
    : isInitialize(false), wsaStarted(false), port(0), iocp(NULL),
      sock(INVALID_SOCKET), running(false), connected(false), socketOpen(false),
	  pendingIo(0), allIoDone(NULL), sendThread(NULL), callbackThread(NULL)
{
    memset(&addr, 0, sizeof(addr));
    ioThread[0] = ioThread[1] = NULL;
}

DRClient::~DRClient()
{
    end();
}

int DRClient::init(const char* ip, int port)
{
    if (isInitialize)
        return 1;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 2;
    wsaStarted = true;

    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    allIoDone = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (iocp == NULL || allIoDone == NULL)
    {
        end();
        return 3;
    }

    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
        end();
        return 4;
    }
	 socketOpen.store(true);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        end();
        return 5;
    }
    addr.sin_port = htons(port);

    this->port = port;
    isInitialize = true;
    return 0;
}

bool DRClient::start()
{
    if (!isInitialize || running)
        return false;

    if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return false;
    if (CreateIoCompletionPort((HANDLE)sock, iocp, (ULONG_PTR)sock, 0) == NULL)
    {
		if (socketOpen.exchange(false))
			closesocket(sock);
        sock = INVALID_SOCKET;
        return false;
    }

    running.store(true);
    connected.store(true);
    ioThread[0] = (HANDLE)_beginthreadex(NULL, 0, IOThreadMain, this, 0, NULL);
    ioThread[1] = (HANDLE)_beginthreadex(NULL, 0, IOThreadMain, this, 0, NULL);
    sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
    callbackThread = (HANDLE)_beginthreadex(NULL, 0, CallbackThread, this, 0, NULL);
    if (ioThread[0] == NULL || ioThread[1] == NULL ||
        sendThread == NULL || callbackThread == NULL)
    {
        end();
        return false;
    }

    auto ioInfo = ioPool.Alloc();
    BeginIo();
    if (!PostRecv(ioInfo))
    {
        ioPool.Dealloc(ioInfo);
        CompleteIo();
        Disconnect();
        end();
        return false;
    }

    OnConnected();
    return true;
}

void DRClient::send_data(char* data, int size)
{
    if (!running || !connected || data == nullptr ||
        size < 0 || size > DRPacket::max_data_size())
        return;

    auto packet = packetPool.Alloc();
    packet->init();
    if (!packet->put(data, size))
    {
        packetPool.Dealloc(packet);
        return;
    }
    packet->header()->size = size;
    packet->header()->code = DRPacket::CODE;
    sendQ.push({ IOtype::SEND, packet->full_size(), packet });
}

void DRClient::wait()
{
    if (callbackThread != NULL)
        WaitForSingleObject(callbackThread, INFINITE);
}

void DRClient::end()
{
    running.store(false);
    Disconnect();

    if (sendThread != NULL)
    {
        WaitForSingleObject(sendThread, INFINITE);
        CloseHandle(sendThread);
        sendThread = NULL;
    }

    if (allIoDone != NULL)
        WaitForSingleObject(allIoDone, INFINITE);

    if (callbackThread != NULL)
    {
        recvQ.push({ IOtype::SHUTDOWN, 0, nullptr });
        WaitForSingleObject(callbackThread, INFINITE);
        CloseHandle(callbackThread);
        callbackThread = NULL;
    }

    if (iocp != NULL)
    {
        for (int i = 0; i < 2; ++i)
            if (ioThread[i] != NULL)
                PostQueuedCompletionStatus(iocp, 0, 0, nullptr);
    }
    for (int i = 0; i < 2; ++i)
    {
        if (ioThread[i] != NULL)
        {
            WaitForSingleObject(ioThread[i], INFINITE);
            CloseHandle(ioThread[i]);
            ioThread[i] = NULL;
        }
    }

    if (iocp != NULL)
    {
        CloseHandle(iocp);
        iocp = NULL;
    }
    if (allIoDone != NULL)
    {
        CloseHandle(allIoDone);
        allIoDone = NULL;
    }
    if (wsaStarted)
    {
        WSACleanup();
        wsaStarted = false;
    }
	 sock = INVALID_SOCKET;
    isInitialize = false;
}

unsigned int _stdcall DRClient::IOThreadMain(void* clClass)
{
    auto cl = (DRClient*)clClass;
    while (true)
    {
        DWORD bytesTrans = 0;
        ULONG_PTR completionKey = 0;
        LPPER_IO_INFO ioInfo = nullptr;
        BOOL success = GetQueuedCompletionStatus(cl->iocp, &bytesTrans,
            &completionKey, (LPOVERLAPPED*)&ioInfo, INFINITE);
        if (ioInfo == nullptr)
            break;

        if (ioInfo->rwMode == IOtype::RECV)
        {
            if (!success || bytesTrans == 0 || !cl->running ||
                !cl->RecvProcess(ioInfo->buffer, (int)bytesTrans))
            {
                cl->ioPool.Dealloc(ioInfo);
                cl->CompleteIo();
                cl->Disconnect();
                continue;
            }

            if (!cl->PostRecv(ioInfo))
            {
                cl->ioPool.Dealloc(ioInfo);
                cl->CompleteIo();
                cl->Disconnect();
            }
        }
        else
        {
            if (!success || bytesTrans == 0 || bytesTrans > ioInfo->wsabuf.len)
            {
                cl->ioPool.Dealloc(ioInfo);
                cl->CompleteIo();
                cl->Disconnect();
                continue;
            }

            if (bytesTrans < ioInfo->wsabuf.len)
            {
                ioInfo->wsabuf.buf += bytesTrans;
                ioInfo->wsabuf.len -= bytesTrans;
                memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
                int result = WSASend(cl->sock, &ioInfo->wsabuf, 1,
                    NULL, 0, &ioInfo->overlapped, NULL);
                if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                {
                    cl->ioPool.Dealloc(ioInfo);
                    cl->CompleteIo();
                    cl->Disconnect();
                }
                continue;
            }

            cl->recvQ.push({ IOtype::SEND, ioInfo->totalBytes, nullptr });
            cl->ioPool.Dealloc(ioInfo);
            cl->CompleteIo();
        }
    }
    return 0;
}

unsigned int _stdcall DRClient::SendThread(void* clClass)
{
    auto cl = (DRClient*)clClass;
    PER_PROCESS_INFO info;
    while (cl->running || cl->sendQ.size() > 0)
    {
        if (!cl->sendQ.pop(&info))
        {
            Sleep(0);
            continue;
        }

        auto ioInfo = cl->ioPool.Alloc();
        memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
        ioInfo->wsabuf.len = info.packet->callPacket(ioInfo->buffer);
        ioInfo->wsabuf.buf = ioInfo->buffer;
        ioInfo->rwMode = IOtype::SEND;
        ioInfo->totalBytes = ioInfo->wsabuf.len;
        cl->packetPool.Dealloc(info.packet);

        if (!cl->running || !cl->connected)
        {
            cl->ioPool.Dealloc(ioInfo);
            continue;
        }

        cl->BeginIo();
        int result = WSASend(cl->sock, &ioInfo->wsabuf, 1,
            NULL, 0, &ioInfo->overlapped, NULL);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            cl->ioPool.Dealloc(ioInfo);
            cl->CompleteIo();
            cl->Disconnect();
        }
    }
    return 0;
}

unsigned int _stdcall DRClient::CallbackThread(void* clClass)
{
    auto cl = (DRClient*)clClass;
    while (true)
    {
        cl->OnUpdate();
        PER_PROCESS_INFO info;
        if (!cl->recvQ.pop(&info))
        {
            Sleep(1);
            continue;
        }

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
        case IOtype::SHUTDOWN:
            return 0;
        }
    }
}

bool DRClient::RecvProcess(char* data, int size)
{
    DRPacket::Header head;
    recvData.Lock();
    if (recvData.Put(data, size) != size)
    {
        recvData.Unlock();
        return false;
    }

    while (recvData.GetUseSize() > 0)
    {
        int useSize = recvData.GetUseSize();
        if (useSize < sizeof(head))
            break;

        recvData.Peek((char*)&head, sizeof(head));
        if (head.code != DRPacket::CODE || head.size < 0 ||
            head.size > DRPacket::max_data_size())
        {
            recvData.ClearBuffer();
            recvData.Unlock();
            return false;
        }

        int packetSize = sizeof(head) + head.size;
        if (useSize < packetSize)
            break;

        auto packet = packetPool.Alloc();
        packet->init();
        if (recvData.Get(packet->getBuf(), packetSize) != packetSize ||
            !packet->movep(packetSize))
        {
            packetPool.Dealloc(packet);
            recvData.Unlock();
            return false;
        }
        recvQ.push({ IOtype::RECV, packet->size(), packet });
    }

    recvData.Unlock();
    return true;
}

bool DRClient::PostRecv(LPPER_IO_INFO ioInfo)
{
    if (!running || !connected)
        return false;

    DWORD flags = 0;
    memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
    ioInfo->wsabuf.len = BUFSIZ;
    ioInfo->wsabuf.buf = ioInfo->buffer;
    ioInfo->rwMode = IOtype::RECV;
    int result = WSARecv(sock, &ioInfo->wsabuf, 1, NULL,
        &flags, &ioInfo->overlapped, NULL);
    return result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING;
}

void DRClient::Disconnect()
{
	running.store(false);
	bool notify = connected.exchange(false);

    if (socketOpen.exchange(false))
    {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
    }
	if (notify)
		recvQ.push({ IOtype::DCON, 0, nullptr });
}

void DRClient::BeginIo()
{
    if (pendingIo.fetch_add(1) == 0)
        ResetEvent(allIoDone);
}

void DRClient::CompleteIo()
{
    if (pendingIo.fetch_sub(1) == 1)
        SetEvent(allIoDone);
}
