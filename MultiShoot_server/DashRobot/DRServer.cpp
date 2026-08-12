#include "stdafx.h"
#include "DRServer.h"

DRServer::DRServer()
    : port(0), iocp(NULL), svrSock(INVALID_SOCKET), threadCnt(0),
      isInitialized(false), wsaStarted(false), running(false), activeClients(0),
      acceptThread(NULL), sendThread(NULL), callbackThread(NULL)
{
}

DRServer::~DRServer()
{
    end();
}

int DRServer::init(int port, int threadCnt)
{
    if (isInitialized)
        return 1;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 2;
    wsaStarted = true;

    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (iocp == NULL)
    {
        end();
        return 3;
    }

    svrSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (svrSock == INVALID_SOCKET)
    {
        end();
        return 4;
    }

    SOCKADDR_IN svrAddr = {};
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svrAddr.sin_port = htons(port);

    if (bind(svrSock, (SOCKADDR*)&svrAddr, sizeof(svrAddr)) == SOCKET_ERROR ||
        listen(svrSock, SOMAXCONN) == SOCKET_ERROR)
    {
        end();
        return 5;
    }

    this->port = port;
    this->threadCnt = threadCnt > 0 ? threadCnt : 1;
    isInitialized = true;
    return 0;
}

void DRServer::start()
{
    if (!isInitialized || running.exchange(true))
        return;

    ioThreads.reserve(threadCnt);
    for (int i = 0; i < threadCnt; ++i)
    {
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, IOThreadMain, this, 0, NULL);
        if (thread == NULL)
        {
            end();
            return;
        }
        ioThreads.push_back(thread);
    }

	sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
	if (sendThread == NULL)
	{
		end();
		return;
	}
	callbackThread = (HANDLE)_beginthreadex(NULL, 0, CallbackThread, this, 0, NULL);
	if (callbackThread == NULL)
	{
		end();
		return;
	}
	acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadMain, this, 0, NULL);
	if (acceptThread == NULL)
        end();
}

void DRServer::end()
{
    bool wasRunning = running.exchange(false);

    if (svrSock != INVALID_SOCKET)
    {
        closesocket(svrSock);
        svrSock = INVALID_SOCKET;
    }

    if (acceptThread != NULL)
    {
        WaitForSingleObject(acceptThread, INFINITE);
        CloseHandle(acceptThread);
        acceptThread = NULL;
    }

    if (callbackThread != NULL)
    {
        recvQ.push({ IOtype::SHUTDOWN, nullptr, 0, nullptr });
        WaitForSingleObject(callbackThread, INFINITE);
        CloseHandle(callbackThread);
        callbackThread = NULL;
    }

    if (sendThread != NULL)
    {
        WaitForSingleObject(sendThread, INFINITE);
        CloseHandle(sendThread);
        sendThread = NULL;
    }

    if (iocp != NULL)
    {
        for (size_t i = 0; i < ioThreads.size(); ++i)
            PostQueuedCompletionStatus(iocp, 0, 0, nullptr);
    }
    for (auto thread : ioThreads)
    {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    ioThreads.clear();

    if (iocp != NULL)
    {
        CloseHandle(iocp);
        iocp = NULL;
    }
    if (wsaStarted)
    {
        WSACleanup();
        wsaStarted = false;
    }

    if (wasRunning || isInitialized)
        isInitialized = false;
}

void DRServer::send_data(SOCKET sock, char* data, int size)
{
    if (!running || data == nullptr || size < 0 || size > DRPacket::max_data_size())
        return;

    auto found = clients.find(sock);
    if (found == clients.end() || found->second->closing)
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

    AddRef(found->second);
    sendQ.push({ IOtype::SEND, found->second, packet->full_size(), packet });
}

unsigned int _stdcall DRServer::AcceptThreadMain(void* svClass)
{
    auto svr = (DRServer*)svClass;
    while (svr->running)
    {
        SOCKADDR_IN clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(svr->svrSock, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET)
        {
            if (!svr->running)
                break;
            continue;
        }

        auto client = new PER_HANDLE_DATA;
        client->hClientSock = clientSock;
        client->clientAddr = clientAddr;
        client->references.store(1);
        client->closing.store(false);
        svr->activeClients.fetch_add(1);

        if (CreateIoCompletionPort((HANDLE)clientSock, svr->iocp,
            (ULONG_PTR)client, 0) == NULL)
        {
            closesocket(clientSock);
            svr->ReleaseClient(client);
            continue;
        }

        // The socket is associated with IOCP before OnAccept can send anything.
		svr->AddRef(client);
        svr->recvQ.push({ IOtype::ACCEPT, client, 0, nullptr });

        auto ioInfo = svr->ioPool.Alloc();
        svr->AddRef(client);
        if (!svr->PostRecv(client, ioInfo))
        {
            svr->ioPool.Dealloc(ioInfo);
            svr->CloseClient(client, true);
            svr->ReleaseClient(client);
        }
    }
    return 0;
}

unsigned int _stdcall DRServer::IOThreadMain(void* svClass)
{
    auto svr = (DRServer*)svClass;
    while (true)
    {
        DWORD bytesTrans = 0;
        LPPER_HANDLE_DATA client = nullptr;
        LPPER_IO_DATA ioInfo = nullptr;
        BOOL success = GetQueuedCompletionStatus(svr->iocp, &bytesTrans,
            (PULONG_PTR)&client, (LPOVERLAPPED*)&ioInfo, INFINITE);

        if (ioInfo == nullptr)
            break;

        if (ioInfo->rwMode == IOtype::RECV)
        {
            if (!success || bytesTrans == 0 || client->closing ||
                !svr->RecvProcess(client, ioInfo->buffer, (int)bytesTrans))
            {
                svr->ioPool.Dealloc(ioInfo);
                svr->CloseClient(client, true);
                svr->ReleaseClient(client);
                continue;
            }

            if (!svr->PostRecv(client, ioInfo))
            {
                svr->ioPool.Dealloc(ioInfo);
                svr->CloseClient(client, true);
                svr->ReleaseClient(client);
            }
        }
        else
        {
            if (!success || bytesTrans == 0 || bytesTrans > ioInfo->wsabuf.len)
            {
                svr->ioPool.Dealloc(ioInfo);
                svr->CloseClient(client, true);
                svr->ReleaseClient(client);
                continue;
            }

            if (bytesTrans < ioInfo->wsabuf.len)
            {
                ioInfo->wsabuf.buf += bytesTrans;
                ioInfo->wsabuf.len -= bytesTrans;
                memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
                int result = WSASend(client->hClientSock, &ioInfo->wsabuf, 1,
                    NULL, 0, &ioInfo->overlapped, NULL);
                if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
                {
                    svr->ioPool.Dealloc(ioInfo);
                    svr->CloseClient(client, true);
                    svr->ReleaseClient(client);
                }
                continue;
            }

            svr->recvQ.push({ IOtype::SEND, client, ioInfo->totalBytes, nullptr });
            svr->ioPool.Dealloc(ioInfo);
            // The I/O reference is transferred to the callback queue.
        }
    }
    return 0;
}

unsigned int _stdcall DRServer::SendThread(void* svClass)
{
    auto svr = (DRServer*)svClass;
    PER_PROCESS_INFO info;
    while (svr->running || svr->sendQ.size() > 0)
    {
        if (!svr->sendQ.pop(&info))
        {
            Sleep(0);
            continue;
        }

        auto packet = info.packet;
        auto ioInfo = svr->ioPool.Alloc();
        memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
        ioInfo->wsabuf.len = packet->callPacket(ioInfo->buffer);
        ioInfo->wsabuf.buf = ioInfo->buffer;
        ioInfo->rwMode = IOtype::SEND;
        ioInfo->totalBytes = ioInfo->wsabuf.len;
        svr->packetPool.Dealloc(packet);

        if (!svr->running || info.client->closing)
        {
            svr->ioPool.Dealloc(ioInfo);
            svr->ReleaseClient(info.client);
            continue;
        }

        int result = WSASend(info.client->hClientSock, &ioInfo->wsabuf, 1,
            NULL, 0, &ioInfo->overlapped, NULL);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            svr->ioPool.Dealloc(ioInfo);
            svr->CloseClient(info.client, true);
            svr->ReleaseClient(info.client);
        }
    }
    return 0;
}

unsigned int _stdcall DRServer::CallbackThread(void* svClass)
{
    auto svr = (DRServer*)svClass;
    ULONGLONG lastTick = GetTickCount64();
    bool shuttingDown = false;

    while (true)
    {
        PER_PROCESS_INFO info;
        bool processed = false;
        while (svr->recvQ.pop(&info))
        {
            processed = true;
            switch (info.type)
            {
            case IOtype::SEND:
                if (!shuttingDown)
                    svr->OnSend(info.client->hClientSock, info.size);
                svr->ReleaseClient(info.client);
                break;
            case IOtype::RECV:
                if (!shuttingDown && !info.client->closing)
                    svr->OnRecv(info.client->hClientSock,
                        info.packet->getCallP(), info.packet->size());
                svr->packetPool.Dealloc(info.packet);
                svr->ReleaseClient(info.client);
                break;
            case IOtype::LEAVE:
            {
                auto found = svr->clients.find(info.client->hClientSock);
                if (found != svr->clients.end() && found->second == info.client)
                {
                    svr->OnLeave(info.client->hClientSock);
                    svr->clients.erase(found);
                    svr->ReleaseClient(info.client);
                }
				svr->ReleaseClient(info.client);
                break;
            }
            case IOtype::ACCEPT:
            {
				if (shuttingDown)
				{
					svr->CloseClient(info.client, false);
					svr->ReleaseClient(info.client);
					svr->ReleaseClient(info.client);
					break;
				}
                auto old = svr->clients.find(info.client->hClientSock);
                if (old != svr->clients.end())
                {
                    svr->OnLeave(old->second->hClientSock);
                    svr->ReleaseClient(old->second);
                }
                svr->clients[info.client->hClientSock] = info.client;
                svr->OnAccept(info.client->hClientSock);
				svr->ReleaseClient(info.client);
                break;
            }
            case IOtype::SHUTDOWN:
                shuttingDown = true;
                for (auto& pair : svr->clients)
                {
                    svr->CloseClient(pair.second, false);
                    svr->OnLeave(pair.second->hClientSock);
                    svr->ReleaseClient(pair.second);
                }
                svr->clients.clear();
                break;
            }
        }

        if (!shuttingDown)
        {
            ULONGLONG currentTick = GetTickCount64();
            svr->OnUpdate((float)(currentTick - lastTick) / 1000.f);
            lastTick = currentTick;
        }
        else if (svr->activeClients.load() == 0 && svr->recvQ.size() == 0)
            break;

        if (!processed)
            Sleep(1);
    }
    return 0;
}

bool DRServer::RecvProcess(LPPER_HANDLE_DATA client, char* data, int size)
{
    auto recvData = &client->recvData;
    DRPacket::Header head;
    recvData->Lock();

    if (recvData->Put(data, size) != size)
    {
        recvData->Unlock();
        return false;
    }

    while (recvData->GetUseSize() > 0)
    {
        int useSize = recvData->GetUseSize();
        if (useSize < sizeof(head))
            break;

        recvData->Peek((char*)&head, sizeof(head));
        if (head.code != DRPacket::CODE || head.size < 0 ||
            head.size > DRPacket::max_data_size())
        {
            recvData->ClearBuffer();
            recvData->Unlock();
            return false;
        }

        int packetSize = sizeof(head) + head.size;
        if (useSize < packetSize)
            break;

        auto packet = packetPool.Alloc();
        packet->init();
        if (recvData->Get(packet->getBuf(), packetSize) != packetSize ||
            !packet->movep(packetSize))
        {
            packetPool.Dealloc(packet);
            recvData->Unlock();
            return false;
        }

        AddRef(client);
        recvQ.push({ IOtype::RECV, client, packet->size(), packet });
    }

    recvData->Unlock();
    return true;
}

bool DRServer::PostRecv(LPPER_HANDLE_DATA client, LPPER_IO_DATA ioInfo)
{
    if (client->closing)
        return false;

    DWORD flags = 0;
    memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
    ioInfo->wsabuf.len = BUFSIZ;
    ioInfo->wsabuf.buf = ioInfo->buffer;
    ioInfo->rwMode = IOtype::RECV;
    int result = WSARecv(client->hClientSock, &ioInfo->wsabuf, 1, NULL,
        &flags, &ioInfo->overlapped, NULL);
    return result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING;
}

void DRServer::CloseClient(LPPER_HANDLE_DATA client, bool notify)
{
    if (client == nullptr || client->closing.exchange(true))
        return;

    shutdown(client->hClientSock, SD_BOTH);
    closesocket(client->hClientSock);
    if (notify)
	{
		AddRef(client);
        recvQ.push({ IOtype::LEAVE, client, 0, nullptr });
	}
}

void DRServer::AddRef(LPPER_HANDLE_DATA client)
{
    client->references.fetch_add(1);
}

void DRServer::ReleaseClient(LPPER_HANDLE_DATA client)
{
    if (client->references.fetch_sub(1) == 1)
    {
        delete client;
        activeClients.fetch_sub(1);
    }
}
