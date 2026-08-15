#include "game/multi_shoot.hpp"

#include "network/packet.hpp"

#include <cstdio>
#include <string>

multi_shoot::~multi_shoot() {
    end();
}

void multi_shoot::on_send(SOCKET socket, int size) {
    std::fprintf(stdout, "[%d] send %d\n", static_cast<int>(socket), size);
}

void multi_shoot::on_accept(SOCKET socket) {
    std::fprintf(stdout, "[%d] accept\n", static_cast<int>(socket));
    const auto id = game_.add_player();
    players_[socket] = id;
    flush_events();
}

void multi_shoot::on_receive(SOCKET socket, char* data, int size) {
    const auto player = players_.find(socket);
    if (player == players_.end())
        return;

    multishoot::protocol::ClientPacket packet;
    if (!packet.ParseFromArray(data, size))
        return;

    switch (packet.payload_case()) {
    case multishoot::protocol::ClientPacket::kChangeDirectionRequest: {
        const auto& direction = packet.change_direction_request().direction();
        game_.change_direction(player->second, {direction.x(), direction.y()});
    } break;
    case multishoot::protocol::ClientPacket::kShootRequest:
        game_.shoot(player->second);
        break;
    default:
        return;
    }
    flush_events();
}

void multi_shoot::on_leave(SOCKET socket) {
    std::fprintf(stdout, "[%d] left\n", static_cast<int>(socket));
    const auto player = players_.find(socket);
    if (player == players_.end())
        return;

    const auto id = player->second;
    players_.erase(player);
    game_.remove_player(id);
    flush_events();
}

void multi_shoot::on_update(float delta_time) {
    game_.update(delta_time);
    flush_events();
}

void multi_shoot::flush_events() {
    for (const auto& event : game_.take_events()) {
        if (event.recipient) {
            for (const auto& [socket, id] : players_) {
                if (id == *event.recipient) {
                    send_packet(socket, event.packet);
                    break;
                }
            }
        } else {
            for (const auto& [socket, id] : players_)
                send_packet(socket, event.packet);
        }

        if (event.packet.payload_case() ==
            multishoot::protocol::ServerPacket::kGameEndResponse) {
            for (auto it = players_.begin(); it != players_.end(); ++it) {
                if (it->second == event.packet.game_end_response().player_id()) {
                    players_.erase(it);
                    break;
                }
            }
        }
    }
}

void multi_shoot::send_packet(SOCKET socket,
                              const multishoot::protocol::ServerPacket& packet) {
    std::string data;
    if (!packet.SerializeToString(&data) ||
        data.size() > static_cast<std::size_t>(dr::packet::max_data_size()))
        return;
    send_data(socket, data.data(), static_cast<int>(data.size()));
}
