#include "game/multi_shoot.hpp"

#include "network/packet.hpp"

#include <algorithm>
#include <cstdio>
#include <type_traits>
#include <utility>

namespace {

bool valid_username(const std::string& username) {
    if (username.size() < 3 || username.size() > 16)
        return false;
    return std::all_of(username.begin(), username.end(), [](unsigned char value) {
        return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
               (value >= '0' && value <= '9') || value == '_';
    });
}

bool valid_password(const std::string& password) {
    if (password.size() < 8 || password.size() > 64)
        return false;
    return std::all_of(password.begin(), password.end(),
                       [](unsigned char value) { return value >= 33 && value <= 126; });
}

} // namespace

multi_shoot::multi_shoot(database_config config) : database_(std::move(config)) {}

multi_shoot::~multi_shoot() {
    end();
    database_.stop();
}

bool multi_shoot::start_database(std::string& error) {
    return database_.start(error);
}

void multi_shoot::on_send(SOCKET socket, int size) {
    std::fprintf(stdout, "[%d] send %d\n", static_cast<int>(socket), size);
}

void multi_shoot::on_accept(SOCKET socket) {
    connection_ids_[socket] = next_connection_id_++;
    std::fprintf(stdout, "[%d] accept\n", static_cast<int>(socket));
}

void multi_shoot::on_receive(SOCKET socket, char* data, int size) {
    multishoot::protocol::ClientPacket packet;
    if (!packet.ParseFromArray(data, size))
        return;

    if (!authenticated_accounts_.contains(socket)) {
        switch (packet.payload_case()) {
        case multishoot::protocol::ClientPacket::kLoginRequest: {
            const auto& request = packet.login_request();
            submit_authentication(socket, request.username(), request.password(), false);
        } break;
        case multishoot::protocol::ClientPacket::kSignupRequest: {
            const auto& request = packet.signup_request();
            submit_authentication(socket, request.username(), request.password(), true);
        } break;
        case multishoot::protocol::ClientPacket::kLeaderboardRequest: {
            const auto connection = connection_ids_.find(socket);
            if (connection == connection_ids_.end())
                return;
            const auto page = packet.leaderboard_request().page();
            if (!database_.submit_leaderboard(socket, connection->second, page))
                send_leaderboard_response(socket, page);
        } break;
        default:
            break;
        }
        return;
    }

    const auto player = players_.find(socket);
    if (player == players_.end())
        return;

    switch (packet.payload_case()) {
    case multishoot::protocol::ClientPacket::kChangeDirectionRequest: {
        const auto& direction = packet.change_direction_request().direction();
        game_.change_direction(player->second, {direction.x(), direction.y()});
    } break;
    case multishoot::protocol::ClientPacket::kShootRequest:
        game_.shoot(player->second);
        break;
    default:
        return;
    }
    flush_events();
}

void multi_shoot::on_leave(SOCKET socket) {
    std::fprintf(stdout, "[%d] left\n", static_cast<int>(socket));
    pending_authentication_.erase(socket);
    connection_ids_.erase(socket);

    const auto account = authenticated_accounts_.find(socket);
    if (account != authenticated_accounts_.end()) {
        online_accounts_.erase(account->second);
        authenticated_accounts_.erase(account);
    }

    const auto player = players_.find(socket);
    if (player == players_.end())
        return;

    const auto id = player->second;
    players_.erase(player);
    game_.remove_player(id);
    flush_events();
}

void multi_shoot::submit_authentication(SOCKET socket, const std::string& username,
                                         const std::string& password, bool signup) {
    using namespace multishoot::protocol;

    if (pending_authentication_.contains(socket))
        return;

    if (!valid_username(username) || !valid_password(password)) {
        send_auth_response(socket, AUTH_RESULT_INVALID_INPUT);
        return;
    }

    const auto connection = connection_ids_.find(socket);
    if (connection == connection_ids_.end())
        return;

    pending_authentication_.insert(socket);
    if (!database_.submit_auth(socket, connection->second, username, password, signup)) {
        pending_authentication_.erase(socket);
        send_auth_response(socket, AUTH_RESULT_SERVER_ERROR);
    }
}

void multi_shoot::process_database_results() {
    database_completion completion;
    while (database_.try_receive(completion)) {
        std::visit(
            [this](const auto& value) {
                using value_type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<value_type, database_auth_completion>)
                    handle_auth_completion(value);
                else if constexpr (std::is_same_v<value_type, database_score_completion>)
                    handle_score_completion(value);
                else
                    handle_leaderboard_completion(value);
            },
            completion);
    }
}

void multi_shoot::handle_auth_completion(const database_auth_completion& completion) {
    const auto connection = connection_ids_.find(completion.socket);
    if (connection == connection_ids_.end() || connection->second != completion.connection_id)
        return;

    pending_authentication_.erase(completion.socket);
    using namespace multishoot::protocol;
    switch (completion.result) {
    case database_auth_result::invalid_credentials:
        send_auth_response(completion.socket, AUTH_RESULT_INVALID_CREDENTIALS);
        return;
    case database_auth_result::username_taken:
        send_auth_response(completion.socket, AUTH_RESULT_USERNAME_TAKEN);
        return;
    case database_auth_result::server_error:
        send_auth_response(completion.socket, AUTH_RESULT_SERVER_ERROR);
        return;
    case database_auth_result::success:
        break;
    }

    if (online_accounts_.contains(completion.username)) {
        send_auth_response(completion.socket, AUTH_RESULT_ACCOUNT_IN_USE);
        return;
    }

    authenticated_accounts_[completion.socket] = completion.username;
    online_accounts_.insert(completion.username);
    send_auth_response(completion.socket, AUTH_RESULT_SUCCESS, completion.best_score);

    players_[completion.socket] = game_.add_player();
    flush_events();
}

void multi_shoot::handle_score_completion(const database_score_completion& completion) {
    if (!completion.success)
        std::fprintf(stderr, "failed to persist score for %s\n", completion.username.c_str());
}

void multi_shoot::handle_leaderboard_completion(
    const database_leaderboard_completion& completion) {
    const auto connection = connection_ids_.find(completion.socket);
    if (connection == connection_ids_.end() || connection->second != completion.connection_id)
        return;
    send_leaderboard_response(completion.socket, completion.page, completion.entries,
                              completion.has_next_page, completion.success);
}

void multi_shoot::send_auth_response(SOCKET socket,
                                     multishoot::protocol::AuthResult result,
                                     std::uint32_t best_score) {
    multishoot::protocol::ServerPacket packet;
    auto* response = packet.mutable_auth_response();
    response->set_result(result);
    response->set_best_score(best_score);
    send_packet(socket, packet);
}

void multi_shoot::send_leaderboard_response(
    SOCKET socket, std::uint32_t page,
    const std::vector<database_leaderboard_entry>& entries, bool has_next_page, bool success) {
    multishoot::protocol::ServerPacket packet;
    auto* response = packet.mutable_leaderboard_response();
    response->set_page(page);
    response->set_has_next_page(has_next_page);
    response->set_success(success);
    for (const auto& entry : entries) {
        auto* item = response->add_entries();
        item->set_rank(entry.rank);
        item->set_username(entry.username);
        item->set_score(entry.score);
    }
    send_packet(socket, packet);
}

void multi_shoot::on_update(float delta_time) {
    process_database_results();
    game_.update(delta_time);
    flush_events();
}

void multi_shoot::flush_events() {
    for (const auto& event : game_.take_events()) {
        if (event.recipient) {
            for (const auto& [socket, id] : players_) {
                if (id == *event.recipient) {
                    send_packet(socket, event.packet);
                    break;
                }
            }
        } else {
            for (const auto& [socket, id] : players_)
                send_packet(socket, event.packet);
        }

        if (event.packet.payload_case() ==
            multishoot::protocol::ServerPacket::kGameEndResponse) {
            const auto player_id = event.packet.game_end_response().player_id();
            for (auto it = players_.begin(); it != players_.end(); ++it) {
                if (it->second != player_id)
                    continue;
                const auto account = authenticated_accounts_.find(it->first);
                if (account != authenticated_accounts_.end() &&
                    !database_.submit_score(account->second,
                                             event.packet.game_end_response().score())) {
                    std::fprintf(stderr, "database score queue is full for %s\n",
                                 account->second.c_str());
                }
                players_.erase(it);
                break;
            }
        }
    }
}

void multi_shoot::send_packet(SOCKET socket,
                              const multishoot::protocol::ServerPacket& packet) {
    std::string data;
    if (!packet.SerializeToString(&data) ||
        data.size() > static_cast<std::size_t>(dr::packet::max_data_size()))
        return;
    send_data(socket, data.data(), static_cast<int>(data.size()));
}
