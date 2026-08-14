#include "controllers/multi_controller.hpp"

#include <iostream>
using std::cout;
using std::endl;
using dr::vector2;

multi_controller::multi_controller() {
    tool_.init("127.0.0.1", 3000);
    work_ = tool_.start();
}

multi_controller::~multi_controller() {
    if (work_) {
        tool_.end();
    }
}

void multi_controller::update() {

    packet_type* packet;
    if (tool_.data_channel_.try_receive(packet)) {
        if (packet == nullptr) {
            cout << "packet nullptr" << endl;
        } else
            enqueue(packet);
    }
}

void multi_controller::change_direction(vector2 dir, Uint32 id) {
    if (work_) {
        change_direction_request req;
        req.dir = dir;
        req.player_id = id;
        req.type = packet_type::change_direction_request;
        tool_.send_data(reinterpret_cast<char*>(&req), sizeof(change_direction_request));
    }
}

void multi_controller::shoot(Uint32 id) {
    if (work_) {
        shoot_request req;
        req.player_id = id;
        req.type = packet_type::shoot_request;
        tool_.send_data(reinterpret_cast<char*>(&req), sizeof(shoot_request));
    }
}
