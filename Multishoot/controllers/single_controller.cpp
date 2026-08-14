#include "controllers/single_controller.hpp"

#include "engine/game_manager.hpp"
#include "engine/player_pref.hpp"
#include "math/rect.hpp"

using dr::rect;
using dr::vector2;

single_controller::single_controller() {
    player_.id = 0;
    player_.dir = vector2(0, 0);
    player_.hp = 5;
    player_.size = vector2(79, 54);
    player_.pos = vector2(260, 500);
    player_.speed = 150;

    enemy_spawn_counter_ = 4;
    enemy_spawn_time_ = 3;

    enemy_id_counter_ = 0;
    bullet_id_counter_ = 0;

    score_ = 0;
    best_score_ = player_pref::get_int("best_score");

    auto login = new login_response;
    login->player_id = 0;
    login->type = packet_type::login_response;
    enqueue(reinterpret_cast<packet_type*>(login));

    auto pspon = new player_spawn_response;
    pspon->player_id = player_.id;
    pspon->pos = vector2(player_.pos);
    pspon->hp = player_.hp;
    pspon->dir = vector2(0, 0);
    pspon->type = packet_type::player_spawn_response;
    enqueue(reinterpret_cast<packet_type*>(pspon));
}

single_controller::~single_controller() {
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        delete (*it);
        it = bullets_.erase(it);
    }

    for (auto it = enemies_.begin(); it != enemies_.end();) {
        delete (*it);
        it = enemies_.erase(it);
    }
}

void single_controller::spawn_enemy() {
    for (int i = 0; i < 5; ++i) {
        auto enemy = new scene_object;
        enemy->id = enemy_id_counter_++;
        enemy->dir = vector2(0, 1);
        enemy->hp = 5;
        enemy->size = vector2(43, 51);
        enemy->pos = vector2(120.f * i + 60.f - 21.5f, 0);
        enemy->speed = 200;
        enemies_.push_back(enemy);

        auto res = new monster_spawn_response;
        res->monster_id = enemy->id;
        res->pos = enemy->pos;
        res->hp = enemy->hp;
        res->type = packet_type::monster_spawn_response;
        enqueue(reinterpret_cast<packet_type*>(res));
    }
}

void single_controller::update_position() {
    auto dt = game_manager::delta_time();
    player_.pos = player_.pos + player_.dir * player_.speed * dt;

    for (auto it = bullets_.begin(); it != bullets_.end();) {
        (*it)->pos = (*it)->pos + (*it)->dir * (*it)->speed * dt;
        if ((*it)->pos.y < -25)
            it = bullets_.erase(it);
        else {
            ++it;
        }
    }

    for (auto it = enemies_.begin(); it != enemies_.end();) {
        auto enemy = *it;
        enemy->pos = enemy->pos + enemy->dir * enemy->speed * dt;
        if (enemy->pos.y > 800 || bullet_enemy_collision(*enemy) ||
            player_enemy_collision(*enemy)) {
            it = enemies_.erase(it);
            delete enemy;
        } else
            ++it;
    }
}

bool single_controller::player_enemy_collision(scene_object& enemy) {
    rect pr(player_.pos, player_.size);

    rect er(enemy.pos, enemy.size);
    if (dr::rect::is_overlapped(pr, er)) {
        auto res = new player_hit_response;
        res->monster_id = enemy.id;
        res->player_health = --player_.hp;
        res->player_id = player_.id;
        res->type = packet_type::player_hit_response;
        enqueue(reinterpret_cast<packet_type*>(res));

        if (player_.hp <= 0) {
            auto res = new game_end_response;
            res->best_score = best_score_ < score_ ? score_ : best_score_;
            res->score = score_;
            res->type = packet_type::game_end_response;
            enqueue(reinterpret_cast<packet_type*>(res));
        }

        return true;
    }
    return false;
}

bool single_controller::bullet_enemy_collision(scene_object& enemy) {
    rect er(enemy.pos, enemy.size);
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        rect br((*it)->pos, (*it)->size);
        if (dr::rect::is_overlapped(er, br)) {
            auto res = new monster_hit_response;
            res->bullet_id = (*it)->id;
            res->monster_health = --enemy.hp;
            res->monster_id = enemy.id;
            res->type = packet_type::monster_hit_response;
            enqueue(reinterpret_cast<packet_type*>(res));

            auto bullet = *it;
            bullets_.erase(it);
            delete bullet;

            if (enemy.hp > 0)
                return false;
            ++score_;
            return true;
        } else
            ++it;
    }
    return false;
}

void single_controller::update() {
    if (enemy_spawn_counter_ >= enemy_spawn_time_) {
        this->spawn_enemy();
        enemy_spawn_counter_ = 0;
    }
    this->update_position();

    enemy_spawn_counter_ += game_manager::delta_time();
}

void single_controller::change_direction(dr::vector2 dir, Uint32 id) {
    player_.dir = dir;

    auto res = new change_direction_response;
    res->dir = dir;
    res->player_id = player_.id;
    res->pos = player_.pos;
    res->type = packet_type::change_direction_response;
    enqueue(reinterpret_cast<packet_type*>(res));
}

void single_controller::shoot(Uint32 id) {
    auto bullet = new scene_object;
    bullet->id = bullet_id_counter_++;
    bullet->dir = vector2(0, -1);
    bullet->size = vector2(24, 8);
    bullet->pos = player_.pos + vector2(28, -4);
    bullet->speed = 300;
    bullet->hp = 0;
    bullets_.push_back(bullet);

    auto res = new shoot_response;
    res->type = packet_type::shoot_response;
    res->bullet_id = bullet->id;
    res->pos = bullet->pos;
    enqueue(reinterpret_cast<packet_type*>(res));
}
