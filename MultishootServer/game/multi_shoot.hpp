#pragma once

#include "database/database_worker.hpp"
#include "game/game_simulation.hpp"
#include "network/server.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

class multi_shoot final : public dr::server {
  public:
    explicit multi_shoot(database_config config);
    ~multi_shoot() override;

    bool start_database(std::string& error);

  protected:
    void on_update(float delta_time) override;
    void on_accept(SOCKET socket) override;
    void on_receive(SOCKET socket, char* data, int size) override;
    void on_leave(SOCKET socket) override;
    void on_send(SOCKET socket, int size) override;

  private:
    multishoot::game_simulation game_;
    database_worker database_;
    std::unordered_map<SOCKET, multishoot::player_id> players_;
    std::unordered_map<SOCKET, std::uint64_t> connection_ids_;
    std::unordered_set<SOCKET> pending_authentication_;
    std::unordered_map<SOCKET, std::string> authenticated_accounts_;
    std::unordered_set<std::string> online_accounts_;
    std::uint64_t next_connection_id_ = 1;

    void submit_authentication(SOCKET socket, const std::string& username,
                               const std::string& password, bool signup);
    void process_database_results();
    void handle_auth_completion(const database_auth_completion& completion);
    void handle_score_completion(const database_score_completion& completion);
    void send_auth_response(SOCKET socket, multishoot::protocol::AuthResult result,
                            std::uint32_t best_score = 0);
    void flush_events();
    void send_packet(SOCKET socket, const multishoot::protocol::ServerPacket& packet);
};
