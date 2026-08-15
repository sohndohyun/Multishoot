#pragma once

#include "math/vector.hpp"
#include "multishoot/protocol/game.pb.h"

#include <cstdint>
#include <list>
#include <optional>
#include <vector>

namespace multishoot {

using player_id = std::uint32_t;

namespace rules {

inline constexpr float screen_width = 600.f;
inline constexpr float screen_height = 800.f;
inline constexpr dr::vector2 player_size{79.f, 54.f};
inline constexpr dr::vector2 enemy_size{43.f, 51.f};
inline constexpr dr::vector2 bullet_size{24.f, 8.f};
inline constexpr dr::vector2 player_start{260.f, 500.f};
inline constexpr float player_speed = 150.f;
inline constexpr float enemy_speed = 200.f;
inline constexpr float bullet_speed = 300.f;
inline constexpr std::uint32_t player_health = 5;
inline constexpr std::uint32_t enemy_health = 5;
inline constexpr int enemies_per_wave = 5;
inline constexpr float enemy_spawn_period = 3.f;
inline constexpr float shoot_cooldown = 0.2f;

} // namespace rules

struct game_event {
    std::optional<player_id> recipient;
    protocol::ServerPacket packet;
};

class game_simulation final {
  public:
    player_id add_player();
    bool remove_player(player_id id);
    bool change_direction(player_id id, dr::vector2 direction);
    bool shoot(player_id id);
    void update(float delta_time);
    std::vector<game_event> take_events();

  private:
    struct scene_object {
        std::uint32_t id{};
        dr::vector2 direction;
        dr::vector2 size;
        dr::vector2 position;
        float speed{};
        std::uint32_t health{};
    };

    struct player_state : scene_object {
        std::uint32_t score{};
        float shoot_cooldown{};
    };

    struct bullet_state : scene_object {
        player_id owner{};
    };

    std::list<player_state> players_;
    std::list<scene_object> enemies_;
    std::list<bullet_state> bullets_;
    std::vector<game_event> events_;

    std::uint32_t player_id_counter_{};
    std::uint32_t enemy_id_counter_{};
    std::uint32_t bullet_id_counter_{};
    float enemy_spawn_counter_{};

    player_state* find_player(player_id id);
    void spawn_enemies();
    bool collide_player(scene_object& enemy);
    bool collide_bullet(scene_object& enemy);
    void reset_session();

    void emit_login(player_id recipient, player_id id);
    void emit_player_spawn(std::optional<player_id> recipient, const player_state& player);
    void emit_direction(const player_state& player);
    void emit_shoot(const bullet_state& bullet);
    void emit_enemy_spawn(std::optional<player_id> recipient, const scene_object& enemy);
    void emit_enemy_hit(const scene_object& enemy, std::uint32_t bullet_id);
    void emit_player_hit(const player_state& player, std::uint32_t enemy_id);
    void emit_game_end(player_id id, std::uint32_t score);
    void emit_player_leave(player_id id);
};

} // namespace multishoot
