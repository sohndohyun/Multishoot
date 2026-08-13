#include "HelloWorld.hpp"

#include "MultiController.h"
#include "SingleController.h"

#include <iostream>

using std::cout;
using std::endl;
using std::to_string;
using tvdr::game_manager;
using tvdr::game_object;
using tvdr::graphics;
using tvdr::input;
using tvdr::player_pref;
using tvdr::text;
using tvdr::vector2;

hello_world::hello_world(game_mode mode) {
    if (mode == game_mode::single)
        controller_ = new single_controller;
    else
        controller_ = new multi_controller;
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

    bullet_cool_time_ = 0.2f;
    bullet_cool_counter_ = 0;

    id_ = -1;
    last_direction_ = vector2(0, 0);
}

void hello_world::update() {
    while (auto packet = controller_->pop()) {
        switch (*packet) {
        case packet_type::login_response: {
            std::cout << "login res" << std::endl;
            auto* res = reinterpret_cast<login_response*>(packet);
            id_ = res->player_id;
            delete res;
        } break;
        case packet_type::player_spawn_response: {
            auto* res = reinterpret_cast<player_spawn_response*>(packet);
            spawn_player(res->player_id, res->pos, res->hp, res->dir);
            delete res;
        } break;
        case packet_type::change_direction_response: {
            auto* res = reinterpret_cast<change_direction_response*>(packet);
            auto p = find_player(res->player_id);
            if (p) {
                p->set_position(res->pos);
                p->set_direction(res->dir);
            }
            delete res;
        } break;
        case packet_type::monster_spawn_response: {
            auto* res = reinterpret_cast<monster_spawn_response*>(packet);
            spawn_enemy(res->monster_id, res->pos, res->hp);
            delete res;
        } break;
        case packet_type::shoot_response: {
            auto* res = reinterpret_cast<shoot_response*>(packet);
            spawn_bullet(res->bullet_id, res->pos);
            delete res;
        } break;
        case packet_type::player_hit_response: {
            auto* res = reinterpret_cast<player_hit_response*>(packet);
            auto p = find_player(res->player_id);
            if (p) {
                p->set_health(res->player_health);
                auto m = find_enemy(res->monster_id);
                if (m)
                    m->set_active(false);
                if (p->health() <= 0)
                    p->set_active(false);
            }
            delete res;
        } break;
        case packet_type::monster_hit_response: {
            auto* res = reinterpret_cast<monster_hit_response*>(packet);
            auto m = find_enemy(res->monster_id);
            if (m) {
                if (m->set_health(res->monster_health)) {
                    m->set_active(false);
                    kill_count_text_->set_text(to_string(++kill_count_));
                }

                auto b = find_bullet(res->bullet_id);
                if (b)
                    b->set_active(false);
            }
            delete res;
        } break;
        case packet_type::game_end_response: {
            std::cout << "game ended" << std::endl;
            auto* res = reinterpret_cast<game_end_response*>(packet);
            player_pref::set_int("best_score", res->best_score);
            player_pref::set_int("score", res->score);
            game_manager::change_scene(new lobby_scene);
            delete res;
        } break;
        default:
            break;
        }
    }

    if (id_ != -1) {
        if (input::is_key_pressed(SDL_SCANCODE_SPACE) && bullet_cool_counter_ > bullet_cool_time_) {
            controller_->shoot(id_);
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
            controller_->change_direction(dir, id_);
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
