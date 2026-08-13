#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "DRPacket.h"
#include "RingBuffer.h"
#include "mpsc_channel.hpp"
#include "object_pool.hpp"

#include <WinSock2.h>
#include <atomic>

class dr_client {
  private:
    enum class io_operation { send, receive, disconnect, shutdown };

    struct io_info {
        OVERLAPPED overlapped;
        char buffer[packet_capacity];
        WSABUF wsabuf;
        io_operation operation;
        int total_bytes;
    };

    struct process_info {
        io_operation type;
        int size;
        dr_packet* packet;
    };

  public:
    dr_client();
    virtual ~dr_client();

    int init(const char* ip, int port);
    bool start();
    void send_data(char* data, int size);
    void wait();
    void end();
    [[nodiscard]] bool is_working() const {
        return running_.load() && connected_.load();
    }

  protected:
    virtual void on_update() = 0;
    virtual void on_connected() = 0;
    virtual void on_send(int size) = 0;
    virtual void on_receive(char* data, int size) = 0;
    virtual void on_disconnected() = 0;

  private:
    static unsigned int _stdcall io_thread_main(void* client_class);
    static unsigned int _stdcall send_thread(void* client_class);
    static unsigned int _stdcall callback_thread(void* client_class);

    bool receive_process(char* data, int size);
    bool post_receive(io_info* io);
    void disconnect();
    void begin_io();
    void complete_io();

    bool is_initialized_;
    bool wsa_started_;
    int port_ = 0;
    HANDLE iocp_ = nullptr;
    SOCKET socket_ = INVALID_SOCKET;
    SOCKADDR_IN address_{};

    dr::object_pool<dr_packet> packet_pool_;
    dr::object_pool<io_info> io_pool_;
    dr::mpsc_channel<process_info> receive_channel_;
    dr::mpsc_channel<process_info> send_channel_;
    dr::ring_buffer receive_buffer_;

    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> socket_open_;
    std::atomic<int> pending_io_;
    HANDLE all_io_done_;
    HANDLE io_threads_[2];
    HANDLE send_thread_handle_;
    HANDLE callback_thread_handle_;
};
