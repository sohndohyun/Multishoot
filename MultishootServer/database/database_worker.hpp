#pragma once

#include "containers/mpsc_channel.hpp"

#include <WinSock2.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

struct database_config final {
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user = "multishoot";
    std::string password = "multishoot_dev";
    std::string name = "multishoot";
};

enum class database_auth_result {
    success,
    invalid_credentials,
    username_taken,
    server_error,
};

struct database_auth_completion final {
    SOCKET socket = INVALID_SOCKET;
    std::uint64_t connection_id = 0;
    std::string username;
    database_auth_result result = database_auth_result::server_error;
    std::uint32_t best_score = 0;
};

struct database_score_completion final {
    std::string username;
    std::uint32_t score = 0;
    bool success = false;
};

struct database_leaderboard_entry final {
    std::uint64_t rank = 0;
    std::string username;
    std::uint32_t score = 0;
};

struct database_leaderboard_completion final {
    SOCKET socket = INVALID_SOCKET;
    std::uint64_t connection_id = 0;
    std::uint32_t page = 0;
    std::vector<database_leaderboard_entry> entries;
    bool has_next_page = false;
    bool success = false;
};

using database_completion = std::variant<database_auth_completion, database_score_completion,
                                         database_leaderboard_completion>;

class database_worker final {
  public:
    explicit database_worker(database_config config);
    database_worker(const database_worker&) = delete;
    database_worker& operator=(const database_worker&) = delete;
    ~database_worker();

    bool start(std::string& error);
    void stop() noexcept;

    bool submit_auth(SOCKET socket, std::uint64_t connection_id, std::string username,
                     std::string password, bool signup);
    bool submit_score(std::string username, std::uint32_t score);
    bool submit_leaderboard(SOCKET socket, std::uint64_t connection_id, std::uint32_t page);
    bool try_receive(database_completion& completion);

  private:
    enum class request_kind { auth, score, leaderboard };

    struct database_request final {
        request_kind kind = request_kind::auth;
        SOCKET socket = INVALID_SOCKET;
        std::uint64_t connection_id = 0;
        std::string username;
        std::string password;
        std::uint32_t score = 0;
        std::uint32_t page = 0;
        bool signup = false;
    };

    static constexpr std::size_t max_pending_requests = 256;

    database_config config_;
    dr::mpsc_channel<database_request> requests_;
    dr::mpsc_channel<database_completion> completions_;
    std::atomic<std::size_t> pending_requests_{0};
    std::jthread thread_;
    std::mutex startup_mutex_;
    std::condition_variable startup_condition_;
    bool startup_done_ = false;
    bool startup_succeeded_ = false;
    std::string startup_error_;

    void run();
    void signal_startup(bool succeeded, std::string error);
    bool enqueue(database_request request);
};
