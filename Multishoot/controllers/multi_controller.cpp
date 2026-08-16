#include "controllers/multi_controller.hpp"

#include <utility>

multi_controller::multi_controller(std::unique_ptr<multi_shoot_client> tool)
    : tool_(std::move(tool)) {}

void multi_controller::update() {
    multishoot::protocol::ServerPacket packet;
    if (tool_->data_channel_.try_receive(packet))
        enqueue(std::move(packet));
}

void multi_controller::change_direction(dr::vector2 direction) {
    multishoot::protocol::ClientPacket packet;
    auto* request = packet.mutable_change_direction_request();
    request->mutable_direction()->set_x(direction.x);
    request->mutable_direction()->set_y(direction.y);
    tool_->send(packet);
}

void multi_controller::shoot() {
    multishoot::protocol::ClientPacket packet;
    packet.mutable_shoot_request();
    tool_->send(packet);
}
