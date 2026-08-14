#pragma once
#include "network/server.hpp"
#include "math/rect.hpp"
#include "math/vector.hpp"

#include <cstdint>
#include <list>

enum class packet_type : std::int32_t {
    change_direction_request = 0,
    shoot_request = 1,
    login_response = 2,
    player_spawn_response = 3,
    change_direction_response = 4,
    shoot_response = 5,
    monster_spawn_response = 6,
    monster_hit_response = 7,
    player_hit_response = 8,
    game_end_response = 9
};

static_assert(static_cast<std::int32_t>(packet_type::game_end_response) == 9);
static_assert(sizeof(packet_type) == sizeof(std::int32_t));

struct change_direction_request {
    packet_type type;
    dr::vector2 dir;
    uint32_t player_id;
};

struct shoot_request {
    packet_type type;
    uint32_t player_id;
};

struct login_response {
    packet_type type;
    uint32_t player_id;
};

struct player_spawn_response {
    packet_type type;
    uint32_t player_id;
    dr::vector2 dir;
    dr::vector2 pos;
    uint32_t hp;
};

struct change_direction_response {
    packet_type type;
    dr::vector2 dir;
    dr::vector2 pos;
    uint32_t player_id;
};

struct shoot_response {
    packet_type type;
    dr::vector2 pos;
    uint32_t bullet_id;
};

struct monster_spawn_response {
    packet_type type;
    dr::vector2 pos;
    uint32_t monster_id;
    uint32_t hp;
};

struct monster_hit_response {
    packet_type type;
    uint32_t monster_id;
    uint32_t bullet_id;
    uint32_t monster_health;
};

struct player_hit_response {
    packet_type type;
    uint32_t player_id;
    uint32_t monster_id;
    uint32_t player_health;
};

struct game_end_response {
    packet_type type;
    uint32_t player_id;
    uint32_t score;
    uint32_t best_score;
};

static_assert(sizeof(change_direction_request) == 16);
static_assert(sizeof(shoot_request) == 8);
static_assert(sizeof(login_response) == 8);
static_assert(sizeof(player_spawn_response) == 28);
static_assert(sizeof(change_direction_response) == 24);
static_assert(sizeof(shoot_response) == 16);
static_assert(sizeof(monster_spawn_response) == 20);
static_assert(sizeof(monster_hit_response) == 16);
static_assert(sizeof(player_hit_response) == 16);
static_assert(sizeof(game_end_response) == 16);

struct scene_object {
    uint32_t id;
    SOCKET socket_;
    dr::vector2 dir;
    dr::vector2 size;
    dr::vector2 pos;
    uint32_t speed;
    uint32_t hp;
    uint32_t score;
};

class multi_shoot final : public dr::server {
  public:
    multi_shoot();
    ~multi_shoot() override;

  protected:
    void on_update(float dt) override;
    void on_accept(SOCKET socket) override;
    void on_receive(SOCKET socket, char* data, int size) override;
    void on_leave(SOCKET socket) override;
    void on_send(SOCKET socket, int size) override;

  private:
    std::list<scene_object*> players_;
    std::list<scene_object*> enemies_;
    std::list<scene_object*> bullets_;

    float enemy_spawn_time_;
    float enemy_spawn_counter_;

    uint32_t enemy_id_counter_;
    uint32_t bullet_id_counter_;
    uint32_t player_id_counter_;

    uint32_t score_;

    void spawn_enemy();
    void update_position(float dt);
    bool player_enemy_collision(scene_object& enemy);
    bool bullet_enemy_collision(scene_object& enemy);
    scene_object* find_player(uint32_t id);

    void send_to_all(char* data, int size);
    void first_data(SOCKET socket);
};
