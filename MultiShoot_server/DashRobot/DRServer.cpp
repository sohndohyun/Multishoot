#include "stdafx.h"
#include "DRServer.h"

DRServer::DRServer()
{
    port = 0;
    iocp = NULL;
    svrSock = INVALID_SOCKET;
    threadCnt = 0;
    isInitialized = false;
}

int DRServer::init(int port, int threadCnt)
{
    if (isInitialized == true)
    {
        return 1;
    }

    WSADATA wsaData;
    SYSTEM_INFO sysInfo;
    SOCKADDR_IN svrAddr;

    this->port = port;
    this->threadCnt = threadCnt;
    this->isInitialized = true;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 2;


    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    svrSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (svrSock == INVALID_SOCKET)
        return 3;

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svrAddr.sin_port = htons(port);

    bind(svrSock, (SOCKADDR*)&svrAddr, sizeof(svrAddr));
    return 0;
}

void DRServer::start()
{
    if (isInitialized == false)
        return;

    _beginthreadex(NULL, 0, AcceptThreadMain, (LPVOID)this, 0, NULL);
    _beginthreadex(NULL, 0, SendThread, (LPVOID)this, 0, NULL);
    _beginthreadex(NULL, 0, CallbackThread, (LPVOID)this, 0, NULL);

    for (int i = 0; i < threadCnt; i++)
        _beginthreadex(NULL, 0, IOThreadMain, (LPVOID)this, 0, NULL);
}

void DRServer::send_data(SOCKET sock, char* data, int size)
{
    auto packet = packetPool.Alloc();
    packet->init();
    packet->put(data, size);
    packet->header()->size = size; 
    packet->header()->code = 12;

    sendQ.push({IOtype::SEND, sock, packet->size(), packet});
}


unsigned int _stdcall DRServer::AcceptThreadMain(void* svClass)
{
    DRServer* svr = (DRServer*)svClass;
    int recvBytes, flags = 0;

    LPPER_IO_DATA ioInfo;
    LPPER_HANDLE_DATA handleInfo;

    listen(svr->svrSock, 5);

    while (true)
    {   
        SOCKET hClientSock;
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);

        hClientSock = accept(svr->svrSock, (SOCKADDR*)&clientAddr, &addrLen);
        handleInfo = new PER_HANDLE_DATA;
        handleInfo->hClientSock = hClientSock;
        memcpy(&handleInfo->clientAddr, &clientAddr, addrLen);

        svr->recvQ.push({ IOtype::ACCEPT, hClientSock, 0, NULL });

        CreateIoCompletionPort((HANDLE)hClientSock, svr->iocp, (ULONG_PTR)handleInfo, 0);

        ioInfo = svr->ioPool.Alloc();

        memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
        ioInfo->wsabuf.len = BUFSIZ;
        ioInfo->wsabuf.buf = ioInfo->buffer;
        ioInfo->rwMode = IOtype::RECV;
        WSARecv(handleInfo->hClientSock, &(ioInfo->wsabuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &ioInfo->overlapped, NULL);
    }
}

unsigned int _stdcall DRServer::IOThreadMain(void* svClass)
{
    DRServer* svr = (DRServer*)svClass;
    SOCKET sock;
    DWORD bytesTrans;
    LPPER_HANDLE_DATA handleInfo;
    LPPER_IO_DATA ioInfo;
    DWORD flags = 0;

    while (true)
    {
        GetQueuedCompletionStatus(svr->iocp, &bytesTrans, (PULONG_PTR)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
        sock = handleInfo->hClientSock;

        if (ioInfo->rwMode == IOtype::RECV)
        {
            if (bytesTrans == 0)
            {
                delete handleInfo;
                svr->ioPool.Dealloc(ioInfo);

                svr->recvQ.push({IOtype::LEAVE, sock, 0, NULL});

                closesocket(sock);
                continue;
            }
            else
            {
                svr->RecvProcess(handleInfo, ioInfo->buffer, bytesTrans);
                
                memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));
                ioInfo->wsabuf.len = BUFSIZ;
                ioInfo->wsabuf.buf = ioInfo->buffer;
                ioInfo->rwMode = IOtype::RECV;
                WSARecv(sock, &ioInfo->wsabuf, 1, NULL, &flags, &ioInfo->overlapped, NULL);
            }
        }
        else
        {
            svr->recvQ.push({ IOtype::SEND, sock, (int)bytesTrans, NULL });
            svr->ioPool.Dealloc(ioInfo);
        }
    }

    return 0;
}

unsigned int _stdcall DRServer::SendThread(void* svClass)
{
    auto svr = (DRServer*)svClass;
    auto sendq = &svr->sendQ;
    while (true)
    {
        if (sendq->size() <= 0)
            continue;

        PER_PROCESS_INFO info;
        sendq->pop(&info);
        
        auto ioInfo = svr->ioPool.Alloc();
        memset(&ioInfo->overlapped, 0, sizeof(OVERLAPPED));

        auto packet = info.packet;
        ioInfo->wsabuf.len = packet->callPacket(ioInfo->buffer);
        ioInfo->wsabuf.buf = ioInfo->buffer;
        ioInfo->rwMode = IOtype::SEND;
        WSASend(info.sock, &ioInfo->wsabuf, 1, NULL, 0, &ioInfo->overlapped, NULL);
    }
    return 0;
}

unsigned int _stdcall DRServer::CallbackThread(void* svClass)
{
    auto svr = (DRServer*)svClass;
    auto recvq = &svr->recvQ;

    ULONGLONG lastTick = GetTickCount64();
    ULONGLONG currentTick;
    while ( true )
    {
        if (recvq->size() > 0) {
            PER_PROCESS_INFO info;
            recvq->pop(&info);

            switch (info.type)
            {
            case IOtype::SEND:
                svr->OnSend(info.sock, info.size);
                break;
            case IOtype::RECV:
                svr->OnRecv(info.sock, info.packet->getCallP(), info.packet->size());
                svr->packetPool.Dealloc(info.packet);
                break;
            case IOtype::LEAVE:
                svr->OnLeave(info.sock);
                break;
            case IOtype::ACCEPT:
                svr->OnAccept(info.sock);
                break;
            }
        }

        currentTick = GetTickCount64();
        svr->OnUpdate((float)(currentTick - lastTick) / 1000.f);
        lastTick = currentTick;
    }
    return 0;
}

void DRServer::RecvProcess(LPPER_HANDLE_DATA client, char* data, int size)
{
    auto recvData = &client->recvData;
    int result;
    DRPacket::Header head;
    recvData->Lock();
    result = recvData->Put(data, size);
    if (result < size) {
        recvData->Unlock();
        return;
    }

    while (auto useSize = recvData->GetUseSize())
    {
        //check header;
        if (useSize < sizeof(head)) {
            recvData->Unlock();
            return;
        }
        recvData->Peek((char*)&head, sizeof(head));

        //check code
        //if(head.code != MYCODE)
        //  return;

        //check size;
        if (useSize < sizeof(head) + head.size) {
            recvData->Unlock();
            return;
        }

        auto packet = packetPool.Alloc();
        packet->init();
        recvData->Get(packet->getBuf(), sizeof(head) + head.size);
        packet->movep(sizeof(head) + head.size);

        recvQ.push({ IOtype::RECV, client->hClientSock, packet->size(), packet });
    }
    recvData->Unlock();
}
