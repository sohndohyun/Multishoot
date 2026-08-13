#pragma once
#include "DRClient.h"
#include "GameController.h"
#include "mpsc_channel.hpp"

class multi_shoot_client final : public dr_client {
  protected:
    void on_update() override;
    void on_connected() override;
    void on_send(int size) override;
    void on_receive(char* data, int size) override;
    void on_disconnected() override;

  public:
    multi_shoot_client();
    ~multi_shoot_client() override;

  public:
    dr::mpsc_channel<packet_type*> data_channel_;
};
