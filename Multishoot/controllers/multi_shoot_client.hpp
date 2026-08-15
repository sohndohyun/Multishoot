#pragma once
#include "network/client.hpp"
#include "controllers/game_controller.hpp"
#include "containers/mpsc_channel.hpp"

class multi_shoot_client final : public dr::client {
  protected:
    void on_update() override;
    void on_connected() override;
    void on_send(int size) override;
    void on_receive(char* data, int size) override;
    void on_disconnected() override;

  public:
    multi_shoot_client();
    ~multi_shoot_client() override;
    void send(const multishoot::protocol::ClientPacket& packet);

  public:
    dr::mpsc_channel<multishoot::protocol::ServerPacket> data_channel_;
};
