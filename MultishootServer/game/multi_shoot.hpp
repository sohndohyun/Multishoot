#pragma once

#include "game/game_simulation.hpp"
#include "network/server.hpp"

#include <unordered_map>

class multi_shoot final : public dr::server {
  public:
    multi_shoot() = default;
    ~multi_shoot() override;

  protected:
    void on_update(float delta_time) override;
    void on_accept(SOCKET socket) override;
    void on_receive(SOCKET socket, char* data, int size) override;
    void on_leave(SOCKET socket) override;
    void on_send(SOCKET socket, int size) override;

  private:
    multishoot::game_simulation game_;
    std::unordered_map<SOCKET, multishoot::player_id> players_;

    void flush_events();
    void send_packet(SOCKET socket, const multishoot::protocol::ServerPacket& packet);
};
