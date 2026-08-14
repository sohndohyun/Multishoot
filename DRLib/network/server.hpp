#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "network/packet.hpp"
#include "containers/ring_buffer.hpp"
#include "containers/mpsc_channel.hpp"
#include "containers/object_pool.hpp"

#include <WinSock2.h>
#include <Windows.h>
#include <atomic>
#include <unordered_map>
#include <vector>

namespace dr {

class server {
  private:
    enum class io_operation { send, receive, leave, accept, shutdown };

    struct client_context {
        SOCKET client_socket;
        SOCKADDR_IN client_address;
        dr::ring_buffer receive_buffer;
        std::atomic<long> references;
        std::atomic<bool> closing;
    };

    struct io_info {
        OVERLAPPED overlapped;
        WSABUF wsabuf;
        char buffer[packet_capacity];
        io_operation operation;
        int total_bytes;
    };

    struct process_info {
        io_operation type;
        client_context* client;
        int size;
        packet* packet;
    };

  public:
    server();
    virtual ~server();

    int init(int port, int thread_count);
    void start();
    void end();

    void send_data(SOCKET socket, char* data, int size);

  protected:
    virtual void on_update(float dt) = 0;
    virtual void on_accept(SOCKET socket) = 0;
    virtual void on_send(SOCKET socket, int size) = 0;
    virtual void on_receive(SOCKET socket, char* data, int size) = 0;
    virtual void on_leave(SOCKET socket) = 0;

  private:
    static unsigned int _stdcall accept_thread_main(void* server_class);
    static unsigned int _stdcall io_thread_main(void* server_class);
    static unsigned int _stdcall send_thread(void* server_class);
    static unsigned int _stdcall callback_thread(void* server_class);

    bool receive_process(client_context* client, char* data, int size);
    bool post_receive(client_context* client, io_info* io);
    void close_client(client_context* client, bool notify);
    void add_reference(client_context* client);
    void release_client(client_context* client);

    int port_ = 0;
    HANDLE iocp_ = nullptr;
    SOCKET server_socket_;
    int thread_count_;

    bool is_initialized_;
    bool wsa_started_;
    std::atomic<bool> running_{false};
    std::atomic<int> active_clients_{0};

    HANDLE accept_thread_;
    HANDLE send_thread_;
    HANDLE callback_thread_;
    std::vector<HANDLE> io_threads_;

    dr::object_pool<io_info> io_pool_;
    dr::object_pool<packet> packet_pool_;

    dr::mpsc_channel<process_info> receive_channel_;
    dr::mpsc_channel<process_info> send_channel_;
    std::unordered_map<SOCKET, client_context*> clients_;
};

} // namespace dr
