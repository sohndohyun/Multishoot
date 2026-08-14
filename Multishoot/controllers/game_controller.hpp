#pragma once

#include "containers/mpsc_channel.hpp"
#include "engine/object.hpp"
#include "SDL.h"
#include "math/vector.hpp"

#include <cstdint>

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
    Uint32 player_id;
};

struct shoot_request {
    packet_type type;
    Uint32 player_id;
};

struct login_response {
    packet_type type;
    Uint32 player_id;
};

struct player_spawn_response {
    packet_type type;
    Uint32 player_id;
    dr::vector2 dir;
    dr::vector2 pos;
    Uint32 hp;
};

struct change_direction_response {
    packet_type type;
    dr::vector2 dir;
    dr::vector2 pos;
    Uint32 player_id;
};

struct shoot_response {
    packet_type type;
    dr::vector2 pos;
    Uint32 bullet_id;
};

struct monster_spawn_response {
    packet_type type;
    dr::vector2 pos;
    Uint32 monster_id;
    Uint32 hp;
};

struct monster_hit_response {
    packet_type type;
    Uint32 monster_id;
    Uint32 bullet_id;
    Uint32 monster_health;
};

struct player_hit_response {
    packet_type type;
    Uint32 player_id;
    Uint32 monster_id;
    Uint32 player_health;
};

struct game_end_response {
    packet_type type;
    Uint32 player_id;
    Uint32 score;
    Uint32 best_score;
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

class game_controller : public object {
  protected:
    dr::mpsc_channel<packet_type*> response_channel_;

    void enqueue(packet_type* packet);

  public:
    virtual void change_direction(dr::vector2 dir, Uint32 id) = 0;
    virtual void shoot(Uint32 id) = 0;
    virtual bool is_working() {
        return true;
    }

    packet_type* pop();
    static void delete_packet(packet_type* pt);
    static packet_type* create_packet(char* data, int size);

    ~game_controller() override;
};
