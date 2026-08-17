#include "database/database_worker.hpp"

#include <bcrypt.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>

#pragma comment(lib, "bcrypt.lib")

namespace {

constexpr unsigned long long password_iterations = 600000;

struct account_record final {
    std::array<unsigned char, 16> salt{};
    std::array<unsigned char, 32> hash{};
    std::uint32_t best_score = 0;
};

bool derive_password_hash(const std::string& password,
                          const std::array<unsigned char, 16>& salt,
                          std::array<unsigned char, 32>& hash) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                    BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
        return false;

    const auto status = BCryptDeriveKeyPBKDF2(
        algorithm, reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
        static_cast<ULONG>(password.size()), const_cast<PUCHAR>(salt.data()),
        static_cast<ULONG>(salt.size()), password_iterations, hash.data(),
        static_cast<ULONG>(hash.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return status >= 0;
}

template <std::size_t Size>
bool secure_equal(const std::array<unsigned char, Size>& left,
                  const std::array<unsigned char, Size>& right) {
    unsigned char difference = 0;
    for (std::size_t i = 0; i < Size; ++i)
        difference |= left[i] ^ right[i];
    return difference == 0;
}

bool connection_error(unsigned int code) {
    return code == CR_CONNECTION_ERROR || code == CR_CONN_HOST_ERROR ||
           code == CR_SERVER_GONE_ERROR || code == CR_SERVER_LOST ||
           code == CR_SERVER_LOST_EXTENDED;
}

struct mysql_deleter final {
    void operator()(MYSQL* connection) const noexcept {
        if (connection)
            mysql_close(connection);
    }
};

struct statement_deleter final {
    void operator()(MYSQL_STMT* statement) const noexcept {
        if (statement)
            mysql_stmt_close(statement);
    }
};

using mysql_ptr = std::unique_ptr<MYSQL, mysql_deleter>;
using statement_ptr = std::unique_ptr<MYSQL_STMT, statement_deleter>;

class worker_database final {
  public:
    explicit worker_database(const database_config& config) : config_(config) {}

    bool connect(std::string& error) {
        mysql_ptr connection(mysql_init(nullptr));
        if (!connection) {
            error = "mysql_init failed";
            return false;
        }

        unsigned int timeout = 5;
        mysql_options(connection.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(connection.get(), MYSQL_OPT_READ_TIMEOUT, &timeout);
        mysql_options(connection.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout);
        bool get_public_key = true;
        mysql_options(connection.get(), MYSQL_OPT_GET_SERVER_PUBLIC_KEY, &get_public_key);

        if (!mysql_real_connect(connection.get(), config_.host.c_str(), config_.user.c_str(),
                                config_.password.c_str(), config_.name.c_str(), config_.port,
                                nullptr, 0)) {
            error = mysql_error(connection.get());
            return false;
        }
        if (mysql_set_character_set(connection.get(), "utf8mb4") != 0) {
            error = mysql_error(connection.get());
            return false;
        }

        connection_ = std::move(connection);
        return validate_schema(error);
    }

    bool reconnect() {
        close();
        std::string ignored;
        return connect(ignored);
    }

    void close() noexcept { connection_.reset(); }

    bool ready() const { return connection_ != nullptr && mysql_ping(connection_.get()) == 0; }

    const std::string& error() const { return last_error_; }

    bool create_account(const std::string& username, const std::string& password,
                        account_record& record, bool& lost_connection,
                        bool& duplicate) {
        lost_connection = false;
        duplicate = false;
        if (BCryptGenRandom(nullptr, record.salt.data(), static_cast<ULONG>(record.salt.size()),
                            BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 ||
            !derive_password_hash(password, record.salt, record.hash))
            return false;

        statement_ptr statement(mysql_stmt_init(connection_.get()));
        if (!statement || !prepare(statement.get(),
                                   "INSERT INTO accounts (username, password_salt, password_hash) "
                                   "VALUES (?, ?, ?)",
                                   lost_connection))
            return false;

        MYSQL_BIND parameters[3]{};
        unsigned long username_length = static_cast<unsigned long>(username.size());
        unsigned long salt_length = static_cast<unsigned long>(record.salt.size());
        unsigned long hash_length = static_cast<unsigned long>(record.hash.size());
        parameters[0].buffer_type = MYSQL_TYPE_STRING;
        parameters[0].buffer = const_cast<char*>(username.data());
        parameters[0].buffer_length = username_length;
        parameters[0].length = &username_length;
        parameters[1].buffer_type = MYSQL_TYPE_BLOB;
        parameters[1].buffer = record.salt.data();
        parameters[1].buffer_length = salt_length;
        parameters[1].length = &salt_length;
        parameters[2].buffer_type = MYSQL_TYPE_BLOB;
        parameters[2].buffer = record.hash.data();
        parameters[2].buffer_length = hash_length;
        parameters[2].length = &hash_length;

        if (mysql_stmt_bind_param(statement.get(), parameters) != 0 ||
            mysql_stmt_execute(statement.get()) != 0) {
            const auto code = mysql_stmt_errno(statement.get());
            lost_connection = connection_error(code);
            duplicate = code == ER_DUP_ENTRY;
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }
        return true;
    }

    bool find_account(const std::string& username, account_record& record, bool& found,
                      bool& lost_connection) {
        found = false;
        lost_connection = false;
        statement_ptr statement(mysql_stmt_init(connection_.get()));
        if (!statement || !prepare(statement.get(),
                                   "SELECT password_salt, password_hash, best_score "
                                   "FROM accounts WHERE username = ? LIMIT 1",
                                   lost_connection))
            return false;

        MYSQL_BIND parameter{};
        unsigned long username_length = static_cast<unsigned long>(username.size());
        parameter.buffer_type = MYSQL_TYPE_STRING;
        parameter.buffer = const_cast<char*>(username.data());
        parameter.buffer_length = username_length;
        parameter.length = &username_length;
        if (mysql_stmt_bind_param(statement.get(), &parameter) != 0 ||
            mysql_stmt_execute(statement.get()) != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }

        unsigned long salt_length = 0;
        unsigned long hash_length = 0;
        unsigned long score_length = 0;
        MYSQL_BIND results[3]{};
        results[0].buffer_type = MYSQL_TYPE_BLOB;
        results[0].buffer = record.salt.data();
        results[0].buffer_length = static_cast<unsigned long>(record.salt.size());
        results[0].length = &salt_length;
        results[1].buffer_type = MYSQL_TYPE_BLOB;
        results[1].buffer = record.hash.data();
        results[1].buffer_length = static_cast<unsigned long>(record.hash.size());
        results[1].length = &hash_length;
        results[2].buffer_type = MYSQL_TYPE_LONG;
        results[2].buffer = &record.best_score;
        results[2].buffer_length = sizeof(record.best_score);
        results[2].length = &score_length;
        results[2].is_unsigned = 1;
        if (mysql_stmt_bind_result(statement.get(), results) != 0 ||
            mysql_stmt_store_result(statement.get()) != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }

        const auto fetch_result = mysql_stmt_fetch(statement.get());
        if (fetch_result == MYSQL_NO_DATA)
            return true;
        if (fetch_result != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }
        if (salt_length != record.salt.size() || hash_length != record.hash.size()) {
            set_error("invalid account record length");
            return false;
        }
        found = true;
        return true;
    }

    bool update_score(const std::string& username, std::uint32_t score, bool& lost_connection) {
        lost_connection = false;
        statement_ptr statement(mysql_stmt_init(connection_.get()));
        if (!statement || !prepare(statement.get(),
                                   "UPDATE accounts SET best_score = GREATEST(best_score, ?) "
                                   "WHERE username = ?",
                                   lost_connection))
            return false;

        MYSQL_BIND parameters[2]{};
        unsigned long username_length = static_cast<unsigned long>(username.size());
        parameters[0].buffer_type = MYSQL_TYPE_LONG;
        parameters[0].buffer = &score;
        parameters[0].buffer_length = sizeof(score);
        parameters[0].is_unsigned = 1;
        parameters[1].buffer_type = MYSQL_TYPE_STRING;
        parameters[1].buffer = const_cast<char*>(username.data());
        parameters[1].buffer_length = username_length;
        parameters[1].length = &username_length;
        if (mysql_stmt_bind_param(statement.get(), parameters) != 0 ||
            mysql_stmt_execute(statement.get()) != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }
        return true;
    }

    bool fetch_leaderboard(std::uint32_t page,
                           std::vector<database_leaderboard_entry>& entries,
                           bool& has_next_page, bool& lost_connection) {
        constexpr std::uint64_t page_size = 10;
        lost_connection = false;
        has_next_page = false;
        entries.clear();

        statement_ptr statement(mysql_stmt_init(connection_.get()));
        if (!statement || !prepare(statement.get(),
                                   "SELECT username, best_score FROM accounts "
                                   "ORDER BY best_score DESC, username ASC LIMIT ? OFFSET ?",
                                   lost_connection))
            return false;

        std::uint64_t limit = page_size + 1;
        std::uint64_t offset = static_cast<std::uint64_t>(page) * page_size;
        MYSQL_BIND parameters[2]{};
        parameters[0].buffer_type = MYSQL_TYPE_LONGLONG;
        parameters[0].buffer = &limit;
        parameters[0].buffer_length = sizeof(limit);
        parameters[0].is_unsigned = 1;
        parameters[1].buffer_type = MYSQL_TYPE_LONGLONG;
        parameters[1].buffer = &offset;
        parameters[1].buffer_length = sizeof(offset);
        parameters[1].is_unsigned = 1;
        if (mysql_stmt_bind_param(statement.get(), parameters) != 0 ||
            mysql_stmt_execute(statement.get()) != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }

        char username[17]{};
        unsigned long username_length = 0;
        std::uint32_t score = 0;
        MYSQL_BIND results[2]{};
        results[0].buffer_type = MYSQL_TYPE_STRING;
        results[0].buffer = username;
        results[0].buffer_length = sizeof(username) - 1;
        results[0].length = &username_length;
        results[1].buffer_type = MYSQL_TYPE_LONG;
        results[1].buffer = &score;
        results[1].buffer_length = sizeof(score);
        results[1].is_unsigned = 1;
        if (mysql_stmt_bind_result(statement.get(), results) != 0 ||
            mysql_stmt_store_result(statement.get()) != 0) {
            lost_connection = connection_error(mysql_stmt_errno(statement.get()));
            set_error(mysql_stmt_error(statement.get()));
            return false;
        }

        for (;;) {
            const auto fetch_result = mysql_stmt_fetch(statement.get());
            if (fetch_result == MYSQL_NO_DATA)
                break;
            if (fetch_result != 0) {
                lost_connection = connection_error(mysql_stmt_errno(statement.get()));
                set_error(mysql_stmt_error(statement.get()));
                return false;
            }
            entries.push_back({offset + entries.size() + 1,
                               std::string(username, username_length), score});
        }
        has_next_page = entries.size() > page_size;
        if (has_next_page)
            entries.resize(page_size);
        return true;
    }

  private:
    bool validate_schema(std::string& error) {
        if (mysql_query(connection_.get(),
                        "SELECT password_salt, password_hash, best_score FROM accounts LIMIT 0") !=
            0) {
            error = mysql_error(connection_.get());
            return false;
        }
        MYSQL_RES* result = mysql_store_result(connection_.get());
        if (result)
            mysql_free_result(result);
        else if (mysql_field_count(connection_.get()) != 0) {
            error = mysql_error(connection_.get());
            return false;
        }
        return true;
    }

    bool prepare(MYSQL_STMT* statement, const char* query, bool& lost_connection) {
        if (mysql_stmt_prepare(statement, query, static_cast<unsigned long>(std::strlen(query))) ==
            0)
            return true;
        lost_connection = connection_error(mysql_stmt_errno(statement));
        set_error(mysql_stmt_error(statement));
        return false;
    }

    void set_error(const char* error) { last_error_ = error ? error : "unknown MySQL error"; }

    database_config config_;
    mysql_ptr connection_;
    std::string last_error_;
};

} // namespace

database_worker::database_worker(database_config config) : config_(std::move(config)) {}

database_worker::~database_worker() {
    stop();
}

bool database_worker::start(std::string& error) {
    if (thread_.joinable()) {
        error = "database worker already started";
        return false;
    }

    {
        std::lock_guard lock(startup_mutex_);
        startup_done_ = false;
        startup_succeeded_ = false;
        startup_error_.clear();
    }
    thread_ = std::jthread([this] { run(); });

    std::unique_lock lock(startup_mutex_);
    startup_condition_.wait(lock, [this] { return startup_done_; });
    if (!startup_succeeded_)
        error = startup_error_;
    return startup_succeeded_;
}

void database_worker::stop() noexcept {
    if (!thread_.joinable())
        return;
    requests_.close();
    thread_.join();
}

bool database_worker::submit_auth(SOCKET socket, std::uint64_t connection_id,
                                  std::string username, std::string password, bool signup) {
    database_request request;
    request.kind = request_kind::auth;
    request.socket = socket;
    request.connection_id = connection_id;
    request.username = std::move(username);
    request.password = std::move(password);
    request.signup = signup;
    return enqueue(std::move(request));
}

bool database_worker::submit_score(std::string username, std::uint32_t score) {
    database_request request;
    request.kind = request_kind::score;
    request.username = std::move(username);
    request.score = score;
    return enqueue(std::move(request));
}

bool database_worker::submit_leaderboard(SOCKET socket, std::uint64_t connection_id,
                                         std::uint32_t page) {
    database_request request;
    request.kind = request_kind::leaderboard;
    request.socket = socket;
    request.connection_id = connection_id;
    request.page = page;
    return enqueue(std::move(request));
}

bool database_worker::try_receive(database_completion& completion) {
    return completions_.try_receive(completion);
}

void database_worker::run() {
    worker_database database(config_);
    std::string error;
    if (!database.connect(error)) {
        signal_startup(false, std::move(error));
        return;
    }
    signal_startup(true, {});

    for (;;) {
        auto request_value = requests_.wait_receive();
        if (!request_value.has_value())
            break;
        auto request = std::move(*request_value);
        struct pending_request_guard final {
            std::atomic<std::size_t>& count;
            ~pending_request_guard() { count.fetch_sub(1, std::memory_order_release); }
        } pending_guard{pending_requests_};

        if (!database.ready() && !database.reconnect()) {
            if (request.kind == request_kind::auth) {
                static_cast<void>(completions_.emplace(database_auth_completion{
                    request.socket, request.connection_id, request.username,
                    database_auth_result::server_error, 0}));
            } else if (request.kind == request_kind::score) {
                static_cast<void>(completions_.emplace(
                    database_score_completion{request.username, request.score, false}));
            } else {
                static_cast<void>(completions_.emplace(database_leaderboard_completion{
                    request.socket, request.connection_id, request.page}));
            }
            continue;
        }

        if (request.kind == request_kind::auth) {
            database_auth_result result = database_auth_result::server_error;
            std::uint32_t best_score = 0;
            bool lost_connection = false;
            if (request.signup) {
                account_record record;
                bool duplicate = false;
                if (database.create_account(request.username, request.password, record,
                                             lost_connection, duplicate)) {
                    result = database_auth_result::success;
                } else if (duplicate) {
                    result = database_auth_result::username_taken;
                }
            } else {
                account_record record;
                bool found = false;
                if (database.find_account(request.username, record, found, lost_connection) &&
                    found) {
                    std::array<unsigned char, 32> candidate{};
                    if (derive_password_hash(request.password, record.salt, candidate) &&
                        secure_equal(candidate, record.hash)) {
                        result = database_auth_result::success;
                        best_score = record.best_score;
                    } else {
                        result = database_auth_result::invalid_credentials;
                    }
                } else if (!lost_connection) {
                    result = database_auth_result::invalid_credentials;
                }
            }

            if (lost_connection && database.reconnect()) {
                if (!request.signup) {
                    account_record record;
                    bool found = false;
                    bool ignored_loss = false;
                    if (database.find_account(request.username, record, found, ignored_loss) &&
                        found) {
                        std::array<unsigned char, 32> candidate{};
                        if (derive_password_hash(request.password, record.salt, candidate) &&
                            secure_equal(candidate, record.hash)) {
                            result = database_auth_result::success;
                            best_score = record.best_score;
                        } else {
                            result = database_auth_result::invalid_credentials;
                        }
                    } else if (!ignored_loss) {
                        result = database_auth_result::invalid_credentials;
                    }
                } else {
                    result = database_auth_result::server_error;
                }
            } else if (lost_connection) {
                result = database_auth_result::server_error;
            }
            static_cast<void>(completions_.emplace(database_auth_completion{
                request.socket, request.connection_id, request.username, result, best_score}));
        } else if (request.kind == request_kind::score) {
            bool lost_connection = false;
            bool success = database.update_score(request.username, request.score, lost_connection);
            if (lost_connection && database.reconnect()) {
                bool ignored_loss = false;
                success = database.update_score(request.username, request.score, ignored_loss);
            }
            static_cast<void>(completions_.emplace(
                database_score_completion{request.username, request.score, success}));
        } else {
            database_leaderboard_completion completion;
            completion.socket = request.socket;
            completion.connection_id = request.connection_id;
            completion.page = request.page;
            bool lost_connection = false;
            completion.success = database.fetch_leaderboard(
                request.page, completion.entries, completion.has_next_page, lost_connection);
            if (lost_connection && database.reconnect()) {
                bool ignored_loss = false;
                completion.success = database.fetch_leaderboard(
                    request.page, completion.entries, completion.has_next_page, ignored_loss);
            }
            static_cast<void>(completions_.emplace(std::move(completion)));
        }
    }
}

void database_worker::signal_startup(bool succeeded, std::string error) {
    {
        std::lock_guard lock(startup_mutex_);
        startup_succeeded_ = succeeded;
        startup_error_ = std::move(error);
        startup_done_ = true;
    }
    startup_condition_.notify_one();
}

bool database_worker::enqueue(database_request request) {
    auto pending = pending_requests_.load(std::memory_order_relaxed);
    while (pending < max_pending_requests &&
           !pending_requests_.compare_exchange_weak(pending, pending + 1,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed)) {
    }
    if (pending >= max_pending_requests)
        return false;
    if (!requests_.send(std::move(request))) {
        pending_requests_.fetch_sub(1, std::memory_order_release);
        return false;
    }
    return true;
}
