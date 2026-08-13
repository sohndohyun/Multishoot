#include "DRServer.h"

#include <cassert>
#include <process.h>

#pragma comment(lib, "WS2_32.lib")

dr_server::dr_server()
    : port_(0), iocp_(nullptr), server_socket_(INVALID_SOCKET), thread_count_(0),
      is_initialized_(false), wsa_started_(false), running_(false), active_clients_(0),
      accept_thread_(nullptr), send_thread_(nullptr), callback_thread_(nullptr) {}

dr_server::~dr_server() {
    end();
}

int dr_server::init(int port, int thread_count) {
    if (is_initialized_)
        return 1;

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return 2;
    wsa_started_ = true;

    iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp_ == nullptr) {
        end();
        return 3;
    }

    server_socket_ = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (server_socket_ == INVALID_SOCKET) {
        end();
        return 4;
    }

    SOCKADDR_IN server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    if (bind(server_socket_, reinterpret_cast<SOCKADDR*>(&server_address),
             sizeof(server_address)) == SOCKET_ERROR ||
        listen(server_socket_, SOMAXCONN) == SOCKET_ERROR) {
        end();
        return 5;
    }

    this->port_ = port;
    this->thread_count_ = thread_count > 0 ? thread_count : 1;
    is_initialized_ = true;
    return 0;
}

void dr_server::start() {
    if (!is_initialized_ || running_.exchange(true))
        return;

    io_threads_.reserve(thread_count_);
    for (int i = 0; i < thread_count_; ++i) {
        HANDLE thread =
            reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, io_thread_main, this, 0, nullptr));
        if (thread == nullptr) {
            end();
            return;
        }
        io_threads_.push_back(thread);
    }

    send_thread_ =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, send_thread, this, 0, nullptr));
    if (send_thread_ == nullptr) {
        end();
        return;
    }
    callback_thread_ =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, callback_thread, this, 0, nullptr));
    if (callback_thread_ == nullptr) {
        end();
        return;
    }
    accept_thread_ =
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, accept_thread_main, this, 0, nullptr));
    if (accept_thread_ == nullptr)
        end();
}

void dr_server::end() {
    bool was_running = running_.exchange(false);
    send_channel_.close();

    if (server_socket_ != INVALID_SOCKET) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }

    if (accept_thread_ != nullptr) {
        WaitForSingleObject(accept_thread_, INFINITE);
        CloseHandle(accept_thread_);
        accept_thread_ = nullptr;
    }

    if (callback_thread_ != nullptr) {
        const bool queued = receive_channel_.send({io_operation::shutdown, nullptr, 0, nullptr});
        assert(queued);
        WaitForSingleObject(callback_thread_, INFINITE);
        CloseHandle(callback_thread_);
        callback_thread_ = nullptr;
    }

    if (send_thread_ != nullptr) {
        WaitForSingleObject(send_thread_, INFINITE);
        CloseHandle(send_thread_);
        send_thread_ = nullptr;
    }

    if (iocp_ != nullptr) {
        for (size_t i = 0; i < io_threads_.size(); ++i)
            PostQueuedCompletionStatus(iocp_, 0, 0, nullptr);
    }
    for (auto thread : io_threads_) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    io_threads_.clear();

    if (iocp_ != nullptr) {
        CloseHandle(iocp_);
        iocp_ = nullptr;
    }
    if (wsa_started_) {
        WSACleanup();
        wsa_started_ = false;
    }

    if (was_running || is_initialized_)
        is_initialized_ = false;
}

void dr_server::send_data(SOCKET socket, char* data, int size) {
    if (!running_ || data == nullptr || size < 0 || size > dr_packet::max_data_size())
        return;

    auto found = clients_.find(socket);
    if (found == clients_.end() || found->second->closing)
        return;

    auto packet = packet_pool_.acquire();
    packet->init();
    if (!packet->put(data, size)) {
        packet_pool_.release(packet);
        return;
    }
    packet->header()->size = size;
    packet->header()->code = dr_packet::code;

    add_reference(found->second);
    if (!send_channel_.send({io_operation::send, found->second, packet->full_size(), packet})) {
        packet_pool_.release(packet);
        release_client(found->second);
    }
}

unsigned int _stdcall dr_server::accept_thread_main(void* server_class) {
    auto* server = static_cast<dr_server*>(server_class);
    while (server->running_) {
        SOCKADDR_IN client_address = {};
        int address_length = sizeof(client_address);
        SOCKET client_socket = accept(
            server->server_socket_, reinterpret_cast<SOCKADDR*>(&client_address), &address_length);
        if (client_socket == INVALID_SOCKET) {
            if (!server->running_)
                break;
            continue;
        }

        auto client = new client_context;
        client->client_socket = client_socket;
        client->client_address = client_address;
        client->references.store(1);
        client->closing.store(false);
        server->active_clients_.fetch_add(1);

        if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket), server->iocp_,
                                   reinterpret_cast<ULONG_PTR>(client), 0) == nullptr) {
            closesocket(client_socket);
            server->release_client(client);
            continue;
        }

        // The socket is associated with IOCP before on_accept can send anything.
        server->add_reference(client);
        if (!server->receive_channel_.send({io_operation::accept, client, 0, nullptr}))
            server->release_client(client);

        auto io_info = server->io_pool_.acquire();
        server->add_reference(client);
        if (!server->post_receive(client, io_info)) {
            server->io_pool_.release(io_info);
            server->close_client(client, true);
            server->release_client(client);
        }
    }
    return 0;
}

unsigned int _stdcall dr_server::io_thread_main(void* server_class) {
    auto* server = static_cast<dr_server*>(server_class);
    while (true) {
        DWORD bytes_trans = 0;
        ULONG_PTR completion_key = 0;
        io_info* io_info = nullptr;
        BOOL success =
            GetQueuedCompletionStatus(server->iocp_, &bytes_trans, &completion_key,
                                      reinterpret_cast<LPOVERLAPPED*>(&io_info), INFINITE);
        auto* client = reinterpret_cast<client_context*>(completion_key);

        if (io_info == nullptr)
            break;

        if (io_info->operation == io_operation::receive) {
            if (!success || bytes_trans == 0 || client->closing ||
                !server->receive_process(client, io_info->buffer, static_cast<int>(bytes_trans))) {
                server->io_pool_.release(io_info);
                server->close_client(client, true);
                server->release_client(client);
                continue;
            }

            if (!server->post_receive(client, io_info)) {
                server->io_pool_.release(io_info);
                server->close_client(client, true);
                server->release_client(client);
            }
        } else {
            if (!success || bytes_trans == 0 || bytes_trans > io_info->wsabuf.len) {
                server->io_pool_.release(io_info);
                server->close_client(client, true);
                server->release_client(client);
                continue;
            }

            if (bytes_trans < io_info->wsabuf.len) {
                io_info->wsabuf.buf += bytes_trans;
                io_info->wsabuf.len -= bytes_trans;
                memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));
                int result = WSASend(client->client_socket, &io_info->wsabuf, 1, nullptr, 0,
                                     &io_info->overlapped, nullptr);
                if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
                    server->io_pool_.release(io_info);
                    server->close_client(client, true);
                    server->release_client(client);
                }
                continue;
            }

            if (!server->receive_channel_.send(
                    {io_operation::send, client, io_info->total_bytes, nullptr}))
                server->release_client(client);
            server->io_pool_.release(io_info);
            // The I/O reference is transferred to the callback queue.
        }
    }
    return 0;
}

unsigned int _stdcall dr_server::send_thread(void* server_class) {
    auto* server = static_cast<dr_server*>(server_class);
    while (auto message = server->send_channel_.wait_receive()) {
        auto info = std::move(*message);

        auto packet = info.packet;
        auto io_info = server->io_pool_.acquire();
        memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));
        io_info->wsabuf.len = packet->call_packet(io_info->buffer);
        io_info->wsabuf.buf = io_info->buffer;
        io_info->operation = io_operation::send;
        io_info->total_bytes = io_info->wsabuf.len;
        server->packet_pool_.release(packet);

        if (!server->running_ || info.client->closing) {
            server->io_pool_.release(io_info);
            server->release_client(info.client);
            continue;
        }

        int result = WSASend(info.client->client_socket, &io_info->wsabuf, 1, nullptr, 0,
                             &io_info->overlapped, nullptr);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            server->io_pool_.release(io_info);
            server->close_client(info.client, true);
            server->release_client(info.client);
        }
    }
    return 0;
}

unsigned int _stdcall dr_server::callback_thread(void* server_class) {
    auto* server = static_cast<dr_server*>(server_class);
    ULONGLONG last_tick = GetTickCount64();
    bool shutting_down = false;

    while (true) {
        process_info info;
        bool processed = false;
        while (server->receive_channel_.try_receive(info)) {
            processed = true;
            switch (info.type) {
            case io_operation::send:
                if (!shutting_down)
                    server->on_send(info.client->client_socket, info.size);
                server->release_client(info.client);
                break;
            case io_operation::receive:
                if (!shutting_down && !info.client->closing)
                    server->on_receive(info.client->client_socket, info.packet->call_pointer(),
                                       info.packet->size());
                server->packet_pool_.release(info.packet);
                server->release_client(info.client);
                break;
            case io_operation::leave: {
                auto found = server->clients_.find(info.client->client_socket);
                if (found != server->clients_.end() && found->second == info.client) {
                    server->on_leave(info.client->client_socket);
                    server->clients_.erase(found);
                    server->release_client(info.client);
                }
                server->release_client(info.client);
                break;
            }
            case io_operation::accept: {
                if (shutting_down) {
                    server->close_client(info.client, false);
                    server->release_client(info.client);
                    server->release_client(info.client);
                    break;
                }
                auto old = server->clients_.find(info.client->client_socket);
                if (old != server->clients_.end()) {
                    server->on_leave(old->second->client_socket);
                    server->release_client(old->second);
                }
                server->clients_[info.client->client_socket] = info.client;
                server->on_accept(info.client->client_socket);
                server->release_client(info.client);
                break;
            }
            case io_operation::shutdown:
                shutting_down = true;
                for (auto& pair : server->clients_) {
                    server->close_client(pair.second, false);
                    server->on_leave(pair.second->client_socket);
                    server->release_client(pair.second);
                }
                server->clients_.clear();
                break;
            }
        }

        if (!shutting_down) {
            ULONGLONG current_tick = GetTickCount64();
            server->on_update(static_cast<float>(current_tick - last_tick) / 1000.f);
            last_tick = current_tick;
        } else if (server->active_clients_.load() == 0 && server->receive_channel_.pending() == 0)
            break;

        if (!processed)
            Sleep(1);
    }
    return 0;
}

bool dr_server::receive_process(client_context* client, char* data, int size) {
    auto receive_buffer = &client->receive_buffer;
    dr_packet::frame_header head;
    receive_buffer->lock();

    if (receive_buffer->write(data, size) != size) {
        receive_buffer->unlock();
        return false;
    }

    while (receive_buffer->used_size() > 0) {
        int used_size = receive_buffer->used_size();
        if (used_size < sizeof(head))
            break;

        receive_buffer->peek(reinterpret_cast<char*>(&head), sizeof(head));
        if (head.code != dr_packet::code || head.size < 0 ||
            head.size > dr_packet::max_data_size()) {
            receive_buffer->clear();
            receive_buffer->unlock();
            return false;
        }

        int packet_size = sizeof(head) + head.size;
        if (used_size < packet_size)
            break;

        auto packet = packet_pool_.acquire();
        packet->init();
        if (receive_buffer->read(packet->buffer(), packet_size) != packet_size ||
            !packet->move_put_pointer(packet_size)) {
            packet_pool_.release(packet);
            receive_buffer->unlock();
            return false;
        }

        add_reference(client);
        if (!receive_channel_.send({io_operation::receive, client, packet->size(), packet})) {
            packet_pool_.release(packet);
            release_client(client);
        }
    }

    receive_buffer->unlock();
    return true;
}

bool dr_server::post_receive(client_context* client, io_info* io) {
    if (client->closing)
        return false;

    DWORD flags = 0;
    memset(&io->overlapped, 0, sizeof(OVERLAPPED));
    io->wsabuf.len = packet_capacity;
    io->wsabuf.buf = io->buffer;
    io->operation = io_operation::receive;
    int result =
        WSARecv(client->client_socket, &io->wsabuf, 1, nullptr, &flags, &io->overlapped, nullptr);
    return result != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING;
}

void dr_server::close_client(client_context* client, bool notify) {
    if (client == nullptr || client->closing.exchange(true))
        return;

    shutdown(client->client_socket, SD_BOTH);
    closesocket(client->client_socket);
    if (notify) {
        add_reference(client);
        if (!receive_channel_.send({io_operation::leave, client, 0, nullptr}))
            release_client(client);
    }
}

void dr_server::add_reference(client_context* client) {
    client->references.fetch_add(1);
}

void dr_server::release_client(client_context* client) {
    if (client->references.fetch_sub(1) == 1) {
        delete client;
        active_clients_.fetch_sub(1);
    }
}
