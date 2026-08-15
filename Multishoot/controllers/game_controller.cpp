#include "controllers/game_controller.hpp"

#include <utility>

void game_controller::enqueue(multishoot::protocol::ServerPacket packet) {
    static_cast<void>(response_channel_.send(std::move(packet)));
}

std::optional<multishoot::protocol::ServerPacket> game_controller::pop() {
    multishoot::protocol::ServerPacket packet;
    if (!response_channel_.try_receive(packet))
        return std::nullopt;
    return packet;
}
