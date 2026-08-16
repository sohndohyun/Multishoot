#include "scenes/hello_world.hpp"

#include "engine/game_manager.hpp"
#include "engine/input.hpp"
#include "controllers/multi_controller.hpp"
#include "engine/player_pref.hpp"
#include "controllers/single_controller.hpp"
#include "game/game_simulation.hpp"

#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;
using std::to_string;
using dr::vector2;

hello_world::hello_world() {
    controller_ = new single_controller;
    if (controller_->is_working()) {
        start();
        add_child(controller_);
    } else
        game_manager::change_scene(new lobby_scene);
}

hello_world::hello_world(std::unique_ptr<multi_shoot_client> client, std::uint32_t best_score)
    : multiplayer_(true), best_score_(best_score) {
    controller_ = new multi_controller(std::move(client));
    if (controller_->is_working()) {
        start();
        add_child(controller_);
    } else
        game_manager::change_scene(new lobby_scene);
}

void hello_world::start() {
    auto background = new game_object("background.bmp");
    background->set_scale(1.5f, 1.5f);
    add_child(background);

    kill_count_ = 0;
    kill_count_text_ = new text("Plaguard-ZVnjx.ttf", 40);

    add_child(kill_count_text_);
    kill_count_text_->set_local_position(20, 20);
    if (multiplayer_) {
        best_score_text_ = new text("Plaguard-ZVnjx.ttf", 28);
        best_score_text_->set_local_position(20, 60);
        best_score_text_->set_text("MULTI BEST: " + to_string(best_score_));
        add_child(best_score_text_);
    }

    bullet_cool_time_ = multishoot::rules::shoot_cooldown;
    bullet_cool_counter_ = bullet_cool_time_;

    logged_in_ = false;
    last_direction_ = vector2(0, 0);
}

void hello_world::update() {
    while (auto packet = controller_->pop()) {
        switch (packet->payload_case()) {
        case multishoot::protocol::ServerPacket::kLoginResponse: {
            std::cout << "login res" << std::endl;
            logged_in_ = true;
        } break;
        case multishoot::protocol::ServerPacket::kPlayerSpawnResponse: {
            const auto& res = packet->player_spawn_response();
            spawn_player(res.player_id(), {res.position().x(), res.position().y()}, res.health(),
                         {res.direction().x(), res.direction().y()});
        } break;
        case multishoot::protocol::ServerPacket::kChangeDirectionResponse: {
            const auto& res = packet->change_direction_response();
            auto p = find_player(res.player_id());
            if (p) {
                p->set_position({res.position().x(), res.position().y()});
                p->set_direction({res.direction().x(), res.direction().y()});
            }
        } break;
        case multishoot::protocol::ServerPacket::kMonsterSpawnResponse: {
            const auto& res = packet->monster_spawn_response();
            spawn_enemy(res.monster_id(), {res.position().x(), res.position().y()}, res.health());
        } break;
        case multishoot::protocol::ServerPacket::kShootResponse: {
            const auto& res = packet->shoot_response();
            spawn_bullet(res.bullet_id(), {res.position().x(), res.position().y()});
        } break;
        case multishoot::protocol::ServerPacket::kPlayerHitResponse: {
            const auto& res = packet->player_hit_response();
            auto p = find_player(res.player_id());
            if (p) {
                p->set_health(res.player_health());
                auto m = find_enemy(res.monster_id());
                if (m)
                    m->set_active(false);
                if (p->health() <= 0)
                    p->set_active(false);
            }
        } break;
        case multishoot::protocol::ServerPacket::kMonsterHitResponse: {
            const auto& res = packet->monster_hit_response();
            auto m = find_enemy(res.monster_id());
            if (m) {
                if (m->set_health(res.monster_health())) {
                    m->set_active(false);
                    kill_count_text_->set_text(to_string(++kill_count_));
                }

                auto b = find_bullet(res.bullet_id());
                if (b)
                    b->set_active(false);
            }
        } break;
        case multishoot::protocol::ServerPacket::kGameEndResponse: {
            std::cout << "game ended" << std::endl;
            const auto score = static_cast<int>(packet->game_end_response().score());
            if (multiplayer_) {
                if (static_cast<std::uint32_t>(score) > best_score_)
                    best_score_ = static_cast<std::uint32_t>(score);
                if (best_score_text_)
                    best_score_text_->set_text("MULTI BEST: " + to_string(best_score_));
            } else {
                player_pref::set_int("best_score",
                                     (std::max)(player_pref::get_int("best_score"), score));
                player_pref::set_int("score", score);
            }
            logged_in_ = false;
            game_manager::change_scene(new lobby_scene);
        } break;
        case multishoot::protocol::ServerPacket::kPlayerLeaveResponse: {
            auto p = find_player(packet->player_leave_response().player_id());
            if (p)
                p->set_active(false);
        } break;
        default:
            break;
        }
    }

    if (logged_in_) {
        if (input::is_key_pressed(SDL_SCANCODE_SPACE) &&
            bullet_cool_counter_ >= bullet_cool_time_) {
            controller_->shoot();
            bullet_cool_counter_ = 0;
        }

        vector2 dir;
        if (input::is_key_pressed(SDL_SCANCODE_UP))
            dir.y = -1;
        if (input::is_key_pressed(SDL_SCANCODE_DOWN))
            dir.y = 1;
        if (input::is_key_pressed(SDL_SCANCODE_LEFT))
            dir.x = -1;
        if (input::is_key_pressed(SDL_SCANCODE_RIGHT))
            dir.x = 1;
        dir.norm();

        if (dir != last_direction_) {
            last_direction_ = dir;
            controller_->change_direction(dir);
        }
    }
    bullet_cool_counter_ += game_manager::delta_time();
}

bullet* hello_world::find_inactive_bullet() {
    for (auto b : bullet_list_)
        if (!b->is_active())
            return b;
    return nullptr;
}

enemy* hello_world::find_inactive_enemy() {
    for (auto e : enemy_list_)
        if (!e->is_active())
            return e;
    return nullptr;
}

void hello_world::spawn_enemy(Uint32 id, vector2 pos, Uint32 hp) {
    auto enemy = find_inactive_enemy();
    if (!enemy) {
        enemy = new ::enemy(id, pos, hp);
        add_child(enemy);
        enemy_list_.push_back(enemy);
    } else {
        enemy->set_active(true);
        enemy->initialize(id, pos, hp);
    }
}

void hello_world::spawn_bullet(Uint32 id, vector2 pos) {
    auto bullet = find_inactive_bullet();
    if (!bullet) {
        bullet = new ::bullet(id, pos);
        add_child(bullet);
        bullet_list_.push_back(bullet);
    } else {
        bullet->set_active(true);
        bullet->initialize(id, pos);
    }
}

void hello_world::spawn_player(Uint32 id, vector2 pos, Uint32 hp, vector2 dir) {
    auto player_object = new ::player(id, pos, hp);
    player_object->set_direction(dir);
    add_child(player_object);
    player_list_.push_back(player_object);
}

player* hello_world::find_player(Uint32 id) {
    for (auto o : player_list_) {
        if (o->id == id)
            return o;
    }
    return nullptr;
}

bullet* hello_world::find_bullet(Uint32 id) {
    for (auto o : bullet_list_) {
        if (o->id == id)
            return o;
    }
    return nullptr;
}

enemy* hello_world::find_enemy(Uint32 id) {
    for (auto o : enemy_list_) {
        if (o->id == id)
            return o;
    }
    return nullptr;
}
