#include "MultiShootClient.h"

#include <iostream>

multi_shoot_client::multi_shoot_client() {}

multi_shoot_client::~multi_shoot_client() {
    end();
    packet_type* packet;
    while (data_channel_.try_receive(packet)) {
        game_controller::delete_packet(packet);
    }
}

void multi_shoot_client::on_update() {}

void multi_shoot_client::on_connected() {}

void multi_shoot_client::on_send(int size) {}

void multi_shoot_client::on_receive(char* data, int size) {
    std::cout << "recv " << size << std::endl;
    auto packet = game_controller::create_packet(data, size);
    if (packet != nullptr && !data_channel_.send(packet))
        game_controller::delete_packet(packet);
}

void multi_shoot_client::on_disconnected() {}
