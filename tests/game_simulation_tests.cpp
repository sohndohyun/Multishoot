#include "game/game_simulation.hpp"
#include "network/packet.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

using multishoot::game_event;
using multishoot::game_simulation;
using multishoot::protocol::ClientPacket;
using multishoot::protocol::ServerPacket;

namespace {

int count(const std::vector<game_event>& events, ServerPacket::PayloadCase type) {
    int result = 0;
    for (const auto& event : events)
        result += event.packet.payload_case() == type;
    return result;
}

void assert_round_trip(const ServerPacket& packet) {
    std::string data;
    assert(packet.SerializeToString(&data));
    assert(data.size() <= static_cast<std::size_t>(dr::packet::max_data_size()));
    ServerPacket parsed;
    assert(parsed.ParseFromString(data));
    assert(parsed.payload_case() == packet.payload_case());
    if (packet.payload_case() == ServerPacket::kAuthResponse) {
        assert(parsed.auth_response().result() == packet.auth_response().result());
        assert(parsed.auth_response().best_score() == packet.auth_response().best_score());
    }
}

void assert_round_trip(const ClientPacket& packet) {
    std::string data;
    assert(packet.SerializeToString(&data));
    assert(data.size() <= static_cast<std::size_t>(dr::packet::max_data_size()));
    ClientPacket parsed;
    assert(parsed.ParseFromString(data));
    assert(parsed.payload_case() == packet.payload_case());
}

void test_all_protocol_messages() {
    ClientPacket direction;
    direction.mutable_change_direction_request()->mutable_direction()->set_x(1.f);
    assert_round_trip(direction);

    ClientPacket shoot;
    shoot.mutable_shoot_request();
    assert_round_trip(shoot);

    ClientPacket login;
    login.mutable_login_request()->set_username("player_one");
    login.mutable_login_request()->set_password("password1");
    assert_round_trip(login);

    ClientPacket signup;
    signup.mutable_signup_request()->set_username("player_two");
    signup.mutable_signup_request()->set_password("password2");
    assert_round_trip(signup);

    std::vector<ServerPacket> packets(10);
    packets[0].mutable_login_response()->set_player_id(1);
    packets[1].mutable_player_spawn_response()->set_player_id(1);
    packets[2].mutable_change_direction_response()->set_player_id(1);
    packets[3].mutable_shoot_response()->set_bullet_id(1);
    packets[4].mutable_monster_spawn_response()->set_monster_id(1);
    packets[5].mutable_monster_hit_response()->set_monster_id(1);
    packets[6].mutable_player_hit_response()->set_player_id(1);
    packets[7].mutable_game_end_response()->set_score(1);
    packets[8].mutable_player_leave_response()->set_player_id(1);
    packets[9].mutable_auth_response()->set_result(
        multishoot::protocol::AUTH_RESULT_SUCCESS);
    packets[9].mutable_auth_response()->set_best_score(123);
    for (const auto& packet : packets)
        assert_round_trip(packet);
}

void test_join_and_protocol() {
    game_simulation game;
    const auto first = game.add_player();
    assert(first == 0);
    auto events = game.take_events();
    assert(count(events, ServerPacket::kLoginResponse) == 1);
    assert(count(events, ServerPacket::kPlayerSpawnResponse) == 1);
    assert(count(events, ServerPacket::kMonsterSpawnResponse) == 5);
    assert(events.front().recipient == first);
    for (const auto& event : events)
        assert_round_trip(event.packet);

    const auto second = game.add_player();
    assert(second == 1);
    events = game.take_events();
    assert(count(events, ServerPacket::kLoginResponse) == 1);
    assert(count(events, ServerPacket::kPlayerSpawnResponse) == 2);
    assert(count(events, ServerPacket::kMonsterSpawnResponse) == 5);
    assert(events.front().recipient == second);
    assert(events[0].packet.payload_case() == ServerPacket::kLoginResponse);
    assert(events[1].packet.payload_case() == ServerPacket::kPlayerSpawnResponse);
    for (std::size_t i = 2; i < 7; ++i)
        assert(events[i].packet.payload_case() == ServerPacket::kMonsterSpawnResponse);
    assert(events[7].packet.payload_case() == ServerPacket::kPlayerSpawnResponse);
    assert(!events[7].recipient);
}

void test_input_and_cooldown() {
    game_simulation game;
    const auto id = game.add_player();
    game.take_events();

    assert(!game.change_direction(id, {std::numeric_limits<float>::infinity(), 0.f}));
    assert(game.change_direction(id, {3.f, 4.f}));
    auto events = game.take_events();
    const auto& direction = events.front().packet.change_direction_response().direction();
    assert(std::abs(direction.x() - 0.6f) < 0.001f);
    assert(std::abs(direction.y() - 0.8f) < 0.001f);

    game.update(10.f);
    assert(game.change_direction(id, {}));
    events = game.take_events();
    const auto& position = events.back().packet.change_direction_response().position();
    assert(position.x() == multishoot::rules::screen_width - multishoot::rules::player_size.x);
    assert(position.y() == multishoot::rules::screen_height - multishoot::rules::player_size.y);

    assert(game.shoot(id));
    assert(!game.shoot(id));
    events = game.take_events();
    assert(count(events, ServerPacket::kShootResponse) == 1);
    game.update(multishoot::rules::shoot_cooldown);
    assert(game.shoot(id));

    game.take_events();
    game.update(multishoot::rules::enemy_spawn_period);
    assert(count(game.take_events(), ServerPacket::kMonsterSpawnResponse) ==
           multishoot::rules::enemies_per_wave);
}

void test_score_death_and_reset() {
    game_simulation game;
    const auto id = game.add_player();
    const auto survivor = game.add_player();
    game.take_events();

    bool scored = false;
    for (int tick = 0; tick < 200 && !scored; ++tick) {
        game.shoot(id);
        game.update(0.05f);
        for (const auto& event : game.take_events()) {
            if (event.packet.payload_case() == ServerPacket::kMonsterHitResponse &&
                event.packet.monster_hit_response().monster_health() == 0)
                scored = true;
        }
    }
    assert(scored);

    bool hit_zero = false;
    bool ended = false;
    for (int tick = 0; tick < 800 && !ended; ++tick) {
        game.update(0.05f);
        for (const auto& event : game.take_events()) {
            if (event.packet.payload_case() == ServerPacket::kPlayerHitResponse &&
                event.packet.player_hit_response().player_id() == id) {
                assert(event.packet.player_hit_response().player_health() <=
                       multishoot::rules::player_health);
                hit_zero = event.packet.player_hit_response().player_health() == 0;
            }
            if (event.packet.payload_case() == ServerPacket::kGameEndResponse) {
                assert(event.recipient == id);
                assert(hit_zero);
                assert(event.packet.game_end_response().score() >= 1);
                ended = true;
            }
        }
    }
    assert(ended);
    assert(game.change_direction(survivor, {}));
}

void test_leave_and_reset() {
    game_simulation game;
    const auto first = game.add_player();
    const auto second = game.add_player();
    game.take_events();
    assert(game.remove_player(first));
    assert(count(game.take_events(), ServerPacket::kPlayerLeaveResponse) == 1);
    assert(game.remove_player(second));
    assert(count(game.take_events(), ServerPacket::kPlayerLeaveResponse) == 1);
    assert(game.add_player() == 0);
}

} // namespace

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    test_all_protocol_messages();
    test_join_and_protocol();
    test_input_and_cooldown();
    test_score_death_and_reset();
    test_leave_and_reset();
    google::protobuf::ShutdownProtobufLibrary();
}
