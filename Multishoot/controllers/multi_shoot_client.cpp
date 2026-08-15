#include "controllers/multi_shoot_client.hpp"

#include "network/packet.hpp"

#include <string>
#include <utility>

multi_shoot_client::multi_shoot_client() {}

multi_shoot_client::~multi_shoot_client() {
    end();
}

void multi_shoot_client::send(const multishoot::protocol::ClientPacket& packet) {
    std::string data;
    if (!packet.SerializeToString(&data) ||
        data.size() > static_cast<std::size_t>(dr::packet::max_data_size()))
        return;
    send_data(data.data(), static_cast<int>(data.size()));
}

void multi_shoot_client::on_update() {}

void multi_shoot_client::on_connected() {}

void multi_shoot_client::on_send(int size) {}

void multi_shoot_client::on_receive(char* data, int size) {
    multishoot::protocol::ServerPacket packet;
    if (packet.ParseFromArray(data, size) &&
        packet.payload_case() != multishoot::protocol::ServerPacket::PAYLOAD_NOT_SET)
        static_cast<void>(data_channel_.send(std::move(packet)));
}

void multi_shoot_client::on_disconnected() {}
