#include "GameController.h"

game_controller::~game_controller() {
    packet_type* packet = nullptr;
    while (response_channel_.try_receive(packet)) {
        delete_packet(packet);
    }
}

void game_controller::enqueue(packet_type* packet) {
    if (packet != nullptr && !response_channel_.send(packet))
        delete_packet(packet);
}

packet_type* game_controller::pop() {
    packet_type* packet = nullptr;
    if (!response_channel_.try_receive(packet))
        return nullptr;
    return packet;
}

void game_controller::delete_packet(packet_type* packet) {
    if (packet == nullptr)
        return;

    switch (*packet) {
    case packet_type::change_direction_request: {
        auto* req = reinterpret_cast<change_direction_request*>(packet);
        delete req;
        break;
    }
    case packet_type::shoot_request: {
        auto* req = reinterpret_cast<shoot_request*>(packet);
        delete req;
        break;
    }
    case packet_type::login_response: {
        auto* res = reinterpret_cast<login_response*>(packet);
        delete res;
    } break;
    case packet_type::player_spawn_response: {
        auto* res = reinterpret_cast<player_spawn_response*>(packet);
        delete res;
    } break;
    case packet_type::change_direction_response: {
        auto* res = reinterpret_cast<change_direction_response*>(packet);
        delete res;
    } break;
    case packet_type::monster_spawn_response: {
        auto* res = reinterpret_cast<monster_spawn_response*>(packet);
        delete res;
    } break;
    case packet_type::shoot_response: {
        auto* res = reinterpret_cast<shoot_response*>(packet);
        delete res;
    } break;
    case packet_type::player_hit_response: {
        auto* res = reinterpret_cast<player_hit_response*>(packet);
        delete res;
    } break;
    case packet_type::monster_hit_response: {
        auto* res = reinterpret_cast<monster_hit_response*>(packet);
        delete res;
    } break;
    case packet_type::game_end_response: {
        auto* res = reinterpret_cast<game_end_response*>(packet);
        delete res;
    } break;
    }
}

packet_type* game_controller::create_packet(char* data, int size) {
    if (data == nullptr || size < sizeof(packet_type))
        return nullptr;

    packet_type* packet = reinterpret_cast<packet_type*>(data);
    switch (*packet) {
    case packet_type::login_response: {
        if (size != sizeof(login_response))
            return nullptr;
        auto* res = reinterpret_cast<login_response*>(data);
        auto rt = new login_response;
        rt->player_id = res->player_id;
        rt->type = packet_type::login_response;
        return reinterpret_cast<packet_type*>(rt);
    }
    case packet_type::player_spawn_response: {
        if (size != sizeof(player_spawn_response))
            return nullptr;
        auto* res = reinterpret_cast<player_spawn_response*>(packet);
        return reinterpret_cast<packet_type*>(new player_spawn_response(*res));
    }
    case packet_type::change_direction_response: {
        if (size != sizeof(change_direction_response))
            return nullptr;
        auto* res = reinterpret_cast<change_direction_response*>(packet);
        return reinterpret_cast<packet_type*>(new change_direction_response(*res));
    }
    case packet_type::monster_spawn_response: {
        if (size != sizeof(monster_spawn_response))
            return nullptr;
        auto* res = reinterpret_cast<monster_spawn_response*>(packet);
        return reinterpret_cast<packet_type*>(new monster_spawn_response(*res));
    }
    case packet_type::shoot_response: {
        if (size != sizeof(shoot_response))
            return nullptr;
        auto* res = reinterpret_cast<shoot_response*>(packet);
        return reinterpret_cast<packet_type*>(new shoot_response(*res));
    }
    case packet_type::player_hit_response: {
        if (size != sizeof(player_hit_response))
            return nullptr;
        auto* res = reinterpret_cast<player_hit_response*>(packet);
        return reinterpret_cast<packet_type*>(new player_hit_response(*res));
    }
    case packet_type::monster_hit_response: {
        if (size != sizeof(monster_hit_response))
            return nullptr;
        auto* res = reinterpret_cast<monster_hit_response*>(packet);
        return reinterpret_cast<packet_type*>(new monster_hit_response(*res));
    }
    case packet_type::game_end_response: {
        if (size != sizeof(game_end_response))
            return nullptr;
        auto* res = reinterpret_cast<game_end_response*>(packet);
        return reinterpret_cast<packet_type*>(new game_end_response(*res));
    }
    }
    return nullptr;
}
