#include "game/game_simulation.hpp"

#include "math/rect.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace multishoot {
namespace {

void set_vector(protocol::Vector2* target, dr::vector2 value) {
    target->set_x(value.x);
    target->set_y(value.y);
}

bool is_finite(dr::vector2 value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

} // namespace

player_id game_simulation::add_player() {
    const bool starts_session = players_.empty();
    const player_id id = player_id_counter_++;

    emit_login(id, id);
    for (const auto& player : players_)
        emit_player_spawn(id, player);
    for (const auto& enemy : enemies_)
        emit_enemy_spawn(id, enemy);
    for (const auto& bullet : bullets_) {
        game_event event{id, {}};
        auto* response = event.packet.mutable_shoot_response();
        set_vector(response->mutable_position(), bullet.position);
        response->set_bullet_id(bullet.id);
        events_.push_back(std::move(event));
    }

    player_state player;
    player.id = id;
    player.direction = {};
    player.size = rules::player_size;
    player.position = rules::player_start;
    player.speed = rules::player_speed;
    player.health = rules::player_health;
    players_.push_back(player);
    emit_player_spawn(std::nullopt, players_.back());

    if (starts_session)
        spawn_enemies();
    return id;
}

bool game_simulation::remove_player(player_id id) {
    const auto found = std::find_if(players_.begin(), players_.end(),
                                    [id](const player_state& player) { return player.id == id; });
    if (found == players_.end())
        return false;

    players_.erase(found);
    emit_player_leave(id);
    if (players_.empty())
        reset_session();
    return true;
}

bool game_simulation::change_direction(player_id id, dr::vector2 direction) {
    auto* player = find_player(id);
    if (player == nullptr || !is_finite(direction))
        return false;

    const float length_squared = direction.x * direction.x + direction.y * direction.y;
    if (length_squared > 1.f) {
        const float length = std::sqrt(length_squared);
        direction.x /= length;
        direction.y /= length;
    }
    player->direction = direction;
    emit_direction(*player);
    return true;
}

bool game_simulation::shoot(player_id id) {
    auto* player = find_player(id);
    if (player == nullptr || player->shoot_cooldown > 0.f)
        return false;

    bullet_state bullet;
    bullet.id = bullet_id_counter_++;
    bullet.owner = id;
    bullet.direction = {0.f, -1.f};
    bullet.size = rules::bullet_size;
    bullet.position = player->position + dr::vector2{28.f, -4.f};
    bullet.speed = rules::bullet_speed;
    bullets_.push_back(bullet);
    player->shoot_cooldown = rules::shoot_cooldown;
    emit_shoot(bullets_.back());
    return true;
}

void game_simulation::update(float delta_time) {
    if (players_.empty() || !std::isfinite(delta_time) || delta_time <= 0.f)
        return;

    for (auto& player : players_) {
        player.shoot_cooldown = std::max(0.f, player.shoot_cooldown - delta_time);
        player.position = player.position + player.direction * player.speed * delta_time;
        player.position.x = std::clamp(player.position.x, 0.f,
                                       rules::screen_width - player.size.x);
        player.position.y = std::clamp(player.position.y, 0.f,
                                       rules::screen_height - player.size.y);
    }

    for (auto it = bullets_.begin(); it != bullets_.end();) {
        it->position = it->position + it->direction * it->speed * delta_time;
        if (it->position.y + it->size.y < 0.f)
            it = bullets_.erase(it);
        else
            ++it;
    }

    for (auto it = enemies_.begin(); it != enemies_.end();) {
        it->position = it->position + it->direction * it->speed * delta_time;
        if (it->position.y > rules::screen_height || collide_bullet(*it) || collide_player(*it))
            it = enemies_.erase(it);
        else
            ++it;

        if (players_.empty()) {
            reset_session();
            return;
        }
    }

    enemy_spawn_counter_ += delta_time;
    if (enemy_spawn_counter_ >= rules::enemy_spawn_period) {
        enemy_spawn_counter_ = 0.f;
        spawn_enemies();
    }
}

std::vector<game_event> game_simulation::take_events() {
    std::vector<game_event> result;
    result.swap(events_);
    return result;
}

game_simulation::player_state* game_simulation::find_player(player_id id) {
    const auto found = std::find_if(players_.begin(), players_.end(),
                                    [id](const player_state& player) { return player.id == id; });
    return found == players_.end() ? nullptr : &*found;
}

void game_simulation::spawn_enemies() {
    for (int i = 0; i < rules::enemies_per_wave; ++i) {
        scene_object enemy;
        enemy.id = enemy_id_counter_++;
        enemy.direction = {0.f, 1.f};
        enemy.size = rules::enemy_size;
        enemy.position = {120.f * i + 60.f - rules::enemy_size.x / 2.f, 0.f};
        enemy.speed = rules::enemy_speed;
        enemy.health = rules::enemy_health;
        enemies_.push_back(enemy);
        emit_enemy_spawn(std::nullopt, enemies_.back());
    }
}

bool game_simulation::collide_player(scene_object& enemy) {
    const dr::rect enemy_bounds(enemy.position, enemy.size);
    for (auto it = players_.begin(); it != players_.end(); ++it) {
        if (!dr::rect::is_overlapped(enemy_bounds, {it->position, it->size}))
            continue;

        if (it->health > 0)
            --it->health;
        emit_player_hit(*it, enemy.id);
        if (it->health == 0) {
            const player_id id = it->id;
            const std::uint32_t score = it->score;
            emit_game_end(id, score);
            players_.erase(it);
        }
        return true;
    }
    return false;
}

bool game_simulation::collide_bullet(scene_object& enemy) {
    const dr::rect enemy_bounds(enemy.position, enemy.size);
    for (auto it = bullets_.begin(); it != bullets_.end(); ++it) {
        if (!dr::rect::is_overlapped(enemy_bounds, {it->position, it->size}))
            continue;

        const player_id owner = it->owner;
        const std::uint32_t bullet_id = it->id;
        if (enemy.health > 0)
            --enemy.health;
        emit_enemy_hit(enemy, bullet_id);
        bullets_.erase(it);
        if (enemy.health == 0) {
            if (auto* player = find_player(owner))
                ++player->score;
            return true;
        }
        return false;
    }
    return false;
}

void game_simulation::reset_session() {
    players_.clear();
    enemies_.clear();
    bullets_.clear();
    player_id_counter_ = 0;
    enemy_id_counter_ = 0;
    bullet_id_counter_ = 0;
    enemy_spawn_counter_ = 0.f;
}

void game_simulation::emit_login(player_id recipient, player_id id) {
    game_event event{recipient, {}};
    event.packet.mutable_login_response()->set_player_id(id);
    events_.push_back(std::move(event));
}

void game_simulation::emit_player_spawn(std::optional<player_id> recipient,
                                        const player_state& player) {
    game_event event{recipient, {}};
    auto* response = event.packet.mutable_player_spawn_response();
    response->set_player_id(player.id);
    set_vector(response->mutable_direction(), player.direction);
    set_vector(response->mutable_position(), player.position);
    response->set_health(player.health);
    events_.push_back(std::move(event));
}

void game_simulation::emit_direction(const player_state& player) {
    game_event event{std::nullopt, {}};
    auto* response = event.packet.mutable_change_direction_response();
    set_vector(response->mutable_direction(), player.direction);
    set_vector(response->mutable_position(), player.position);
    response->set_player_id(player.id);
    events_.push_back(std::move(event));
}

void game_simulation::emit_shoot(const bullet_state& bullet) {
    game_event event{std::nullopt, {}};
    auto* response = event.packet.mutable_shoot_response();
    set_vector(response->mutable_position(), bullet.position);
    response->set_bullet_id(bullet.id);
    events_.push_back(std::move(event));
}

void game_simulation::emit_enemy_spawn(std::optional<player_id> recipient,
                                       const scene_object& enemy) {
    game_event event{recipient, {}};
    auto* response = event.packet.mutable_monster_spawn_response();
    set_vector(response->mutable_position(), enemy.position);
    response->set_monster_id(enemy.id);
    response->set_health(enemy.health);
    events_.push_back(std::move(event));
}

void game_simulation::emit_enemy_hit(const scene_object& enemy, std::uint32_t bullet_id) {
    game_event event{std::nullopt, {}};
    auto* response = event.packet.mutable_monster_hit_response();
    response->set_monster_id(enemy.id);
    response->set_bullet_id(bullet_id);
    response->set_monster_health(enemy.health);
    events_.push_back(std::move(event));
}

void game_simulation::emit_player_hit(const player_state& player, std::uint32_t enemy_id) {
    game_event event{std::nullopt, {}};
    auto* response = event.packet.mutable_player_hit_response();
    response->set_player_id(player.id);
    response->set_monster_id(enemy_id);
    response->set_player_health(player.health);
    events_.push_back(std::move(event));
}

void game_simulation::emit_game_end(player_id id, std::uint32_t score) {
    game_event event{id, {}};
    auto* response = event.packet.mutable_game_end_response();
    response->set_player_id(id);
    response->set_score(score);
    events_.push_back(std::move(event));
}

void game_simulation::emit_player_leave(player_id id) {
    game_event event{std::nullopt, {}};
    event.packet.mutable_player_leave_response()->set_player_id(id);
    events_.push_back(std::move(event));
}

} // namespace multishoot
