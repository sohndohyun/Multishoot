#include "network/client.hpp"

#include <WinSock2.h>
#include <cassert>
#include <process.h>
#include <stdio.h>
#include <ws2tcpip.h>

#pragma comment(lib, "WS2_32.lib")

namespace dr {

client::client()
    : is_initialized_(false), wsa_started_(false), port_(0), iocp_(nullptr),
      socket_(INVALID_SOCKET), running_(false), connected_(false), socket_open_(false),
      pending_io_(0), all_io_done_(nullptr), send_thread_handle_(nullptr),
      callback_thread_handle_(nullptr) {
    memset(&address_, 0, sizeof(address_));
    io_threads_[0] = io_threads_[1] = nullptr;
}

client::~client() {
    end();
}

int client::init(const char* ip, int port) {
    if (is_initialized_)
        return 1;

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return 2;
    wsa_started_ = true;

    iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    all_io_done_ = CreateEvent(nullptr, TRUE, TRUE, nullptr);
    if (iocp_ == nullptr || all_io_done_ == nullptr) {
        end();
        return 3;
    }

    socket_ = WSASocketW(PF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket_ == INVALID_SOCKET) {
        end();
        return 4;
    }
    socket_open_.store(true);

    memset(&address_, 0, sizeof(address_));
    address_.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &address_.sin_addr) != 1) {
        end();
        return 5;
    }
    address_.sin_port = htons(port);

    this->port_ = port;
    is_initialized_ = true;
    return 0;
}

bool client::start() {
    if (!is_initialized_ || running_)
        return false;

    if (connect(socket_, reinterpret_cast<SOCKADDR*>(&address_), sizeof(address_)) == SOCKET_ERROR)
        return false;
    if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket_), iocp_,
                               static_cast<ULONG_PTR>(socket_), 0) == nullptr) {
        if (socket_open_.exchange(false))
            closesocket(socket_);
        socket_ = INVALID_SOCKET;
        return false;
    }

    running_.store(true);
    connected_.store(true);
    io_threads_[0] =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, io_thread_main, this, 0, nullptr));
    io_threads_[1] =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, io_thread_main, this, 0, nullptr));
    send_thread_handle_ =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, send_thread, this, 0, nullptr));
    callback_thread_handle_ =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, callback_thread, this, 0, nullptr));
    if (io_threads_[0] == nullptr || io_threads_[1] == nullptr || send_thread_handle_ == nullptr ||
        callback_thread_handle_ == nullptr) {
        end();
        return false;
    }

    auto io_info = io_pool_.acquire();
    begin_io();
    if (!post_receive(io_info)) {
        io_pool_.release(io_info);
        complete_io();
        disconnect();
        end();
        return false;
    }

    on_connected();
    return true;
}

void client::send_data(char* data, int size) {
    if (!running_ || !connected_ || data == nullptr || size < 0 ||
        size > packet::max_data_size())
        return;

    auto packet = packet_pool_.acquire();
    packet->init();
    if (!packet->put(data, size)) {
        packet_pool_.release(packet);
        return;
    }
    packet->header()->size = size;
    packet->header()->code = packet::code;
    if (!send_channel_.send({io_operation::send, packet->full_size(), packet}))
        packet_pool_.release(packet);
}

void client::wait() {
    if (callback_thread_handle_ != nullptr)
        WaitForSingleObject(callback_thread_handle_, INFINITE);
}

void client::end() {
    running_.store(false);
    disconnect();
    send_channel_.close();

    if (send_thread_handle_ != nullptr) {
        WaitForSingleObject(send_thread_handle_, INFINITE);
        CloseHandle(send_thread_handle_);
        send_thread_handle_ = nullptr;
    }

    if (all_io_done_ != nullptr)
        WaitForSingleObject(all_io_done_, INFINITE);

    if (callback_thread_handle_ != nullptr) {
        const bool queued = receive_channel_.send({io_operation::shutdown, 0, nullptr});
        assert(queued);
        WaitForSingleObject(callback_thread_handle_, INFINITE);
        CloseHandle(callback_thread_handle_);
        callback_thread_handle_ = nullptr;
    }

    if (iocp_ != nullptr) {
        for (int i = 0; i < 2; ++i)
            if (io_threads_[i] != nullptr)
                PostQueuedCompletionStatus(iocp_, 0, 0, nullptr);
    }
    for (int i = 0; i < 2; ++i) {
        if (io_threads_[i] != nullptr) {
            WaitForSingleObject(io_threads_[i], INFINITE);
            CloseHandle(io_threads_[i]);
            io_threads_[i] = nullptr;
        }
    }

    if (iocp_ != nullptr) {
        CloseHandle(iocp_);
        iocp_ = nullptr;
    }
    if (all_io_done_ != nullptr) {
        CloseHandle(all_io_done_);
        all_io_done_ = nullptr;
    }
    if (wsa_started_) {
        WSACleanup();
        wsa_started_ = false;
    }
    socket_ = INVALID_SOCKET;
    is_initialized_ = false;
}

unsigned int _stdcall client::io_thread_main(void* client_class) {
    auto* client = static_cast<dr::client*>(client_class);
    while (true) {
        DWORD bytes_trans = 0;
        ULONG_PTR completion_key = 0;
        io_info* io_info = nullptr;
        BOOL success =
            GetQueuedCompletionStatus(client->iocp_, &bytes_trans, &completion_key,
                                      reinterpret_cast<LPOVERLAPPED*>(&io_info), INFINITE);
        if (io_info == nullptr)
            break;

        if (io_info->operation == io_operation::receive) {
            if (!success || bytes_trans == 0 || !client->running_ ||
                !client->receive_process(io_info->buffer, static_cast<int>(bytes_trans))) {
                client->io_pool_.release(io_info);
                client->complete_io();
                client->disconnect();
                continue;
            }

            if (!client->post_receive(io_info)) {
                client->io_pool_.release(io_info);
                client->complete_io();
                client->disconnect();
            }
        } else {
            if (!success || bytes_trans == 0 || bytes_trans > io_info->wsabuf.len) {
                client->io_pool_.release(io_info);
                client->complete_io();
                client->disconnect();
                continue;
            }

            if (bytes_trans < io_info->wsabuf.len) {
                io_info->wsabuf.buf += bytes_trans;
                io_info->wsabuf.len -= bytes_trans;
                memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));
                int result = WSASend(client->socket_, &io_info->wsabuf, 1, nullptr, 0,
                                     &io_info->overlapped, nullptr);
                if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
                    client->io_pool_.release(io_info);
                    client->complete_io();
                    client->disconnect();
                }
                continue;
            }

            const bool queued =
                client->receive_channel_.send({io_operation::send, io_info->total_bytes, nullptr});
            assert(queued);
            client->io_pool_.release(io_info);
            client->complete_io();
        }
    }
    return 0;
}

unsigned int _stdcall client::send_thread(void* client_class) {
    auto* client = static_cast<dr::client*>(client_class);
    while (auto message = client->send_channel_.wait_receive()) {
        auto info = std::move(*message);

        auto io_info = client->io_pool_.acquire();
        memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));
        io_info->wsabuf.len = info.packet->call_packet(io_info->buffer);
        io_info->wsabuf.buf = io_info->buffer;
        io_info->operation = io_operation::send;
        io_info->total_bytes = io_info->wsabuf.len;
        client->packet_pool_.release(info.packet);

        if (!client->running_ || !client->connected_) {
            client->io_pool_.release(io_info);
            continue;
        }

        client->begin_io();
        int result = WSASend(client->socket_, &io_info->wsabuf, 1, nullptr, 0, &io_info->overlapped,
                             nullptr);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            client->io_pool_.release(io_info);
            client->complete_io();
            client->disconnect();
        }
    }
    return 0;
}

unsigned int _stdcall client::callback_thread(void* client_class) {
    auto* client = static_cast<dr::client*>(client_class);
    while (true) {
        client->on_update();
        process_info info;
        if (!client->receive_channel_.try_receive(info)) {
            Sleep(1);
            continue;
        }

        switch (info.type) {
        case io_operation::send:
            client->on_send(info.size);
            break;
        case io_operation::receive:
            client->on_receive(info.packet->call_pointer(), info.size);
            client->packet_pool_.release(info.packet);
            break;
        case io_operation::disconnect:
            client->on_disconnected();
            break;
        case io_operation::shutdown:
            return 0;
        }
    }
}

bool client::receive_process(char* data, int size) {
    packet::frame_header head;
    receive_buffer_.lock();
    if (receive_buffer_.write(data, size) != size) {
        receive_buffer_.unlock();
        return false;
    }

    while (receive_buffer_.used_size() > 0) {
        int used_size = receive_buffer_.used_size();
        if (used_size < sizeof(head))
            break;

        receive_buffer_.peek(reinterpret_cast<char*>(&head), sizeof(head));
        if (head.code != packet::code || head.size < 0 ||
            head.size > packet::max_data_size()) {
            receive_buffer_.clear();
            receive_buffer_.unlock();
            return false;
        }

        int packet_size = sizeof(head) + head.size;
        if (used_size < packet_size)
            break;

        auto packet = packet_pool_.acquire();
        packet->init();
        if (receive_buffer_.read(packet->buffer(), packet_size) != packet_size ||
            !packet->move_put_pointer(packet_size)) {
            packet_pool_.release(packet);
            receive_buffer_.unlock();
            return false;
        }
        if (!receive_channel_.send({io_operation::receive, packet->size(), packet}))
            packet_pool_.release(packet);
    }

    receive_buffer_.unlock();
    return true;
}

bool client::post_receive(io_info* io) {
    if (!running_ || !connected_)
        return false;

    DWORD flags = 0;
    memset(&io->overlapped, 0, sizeof(OVERLAPPED));
    io->wsabuf.len = packet_capacity;
    io->wsabuf.buf = io->buffer;
    io->operation = io_operation::receive;
    int result = WSARecv(socket_, &io->wsabuf, 1, nullptr, &flags, &io->overlapped, nullptr);
    return result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING;
}

void client::disconnect() {
    running_.store(false);
    bool notify = connected_.exchange(false);

    if (socket_open_.exchange(false)) {
        shutdown(socket_, SD_BOTH);
        closesocket(socket_);
    }
    if (notify) {
        const bool queued = receive_channel_.send({io_operation::disconnect, 0, nullptr});
        assert(queued);
    }
}

void client::begin_io() {
    if (pending_io_.fetch_add(1) == 0)
        ResetEvent(all_io_done_);
}

void client::complete_io() {
    if (pending_io_.fetch_sub(1) == 1)
        SetEvent(all_io_done_);
}

} // namespace dr
