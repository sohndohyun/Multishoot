#include "MultiShoot.h"

#include "stdafx.h"

#include <iostream>

using tvdr::rect;
using tvdr::vector2;

multi_shoot::multi_shoot() {
    enemy_spawn_counter_ = 4;
    enemy_spawn_time_ = 3;

    enemy_id_counter_ = 0;
    bullet_id_counter_ = 0;
    player_id_counter_ = 0;

    score_ = 0;
}

multi_shoot::~multi_shoot() {
    end();
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        delete (*it);
        it = bullets_.erase(it);
    }

    for (auto it = enemies_.begin(); it != enemies_.end();) {
        delete (*it);
        it = enemies_.erase(it);
    }

    for (auto it = players_.begin(); it != players_.end();) {
        delete (*it);
        it = players_.erase(it);
    }
}

void multi_shoot::on_send(SOCKET socket, int size) {
    fprintf(stdout, "[%d] send %d \n", static_cast<int>(socket), size);
}

void multi_shoot::on_accept(SOCKET socket) {
    fprintf(stdout, "[%d] accept \n", static_cast<int>(socket));
    scene_object* obj = new scene_object;
    obj->id = player_id_counter_++;
    obj->dir = vector2(0, 0);
    obj->hp = 5;
    obj->size = vector2(79, 54);
    obj->pos = vector2(260, 500);
    obj->speed = 150;
    obj->socket_ = socket;
    obj->score = 0;

    login_response res;
    res.player_id = obj->id;
    res.type = packet_type::login_response;
    send_data(socket, reinterpret_cast<char*>(&res), sizeof(res));

    first_data(socket);
    players_.push_back(obj);

    player_spawn_response pres;
    pres.hp = obj->hp;
    pres.player_id = obj->id;
    pres.pos = obj->pos;
    pres.dir = obj->dir;
    pres.type = packet_type::player_spawn_response;
    send_to_all(reinterpret_cast<char*>(&pres), sizeof(pres));
}

void multi_shoot::on_receive(SOCKET socket, char* data, int size) {
    if (data == nullptr || size < sizeof(packet_type))
        return;

    packet_type* packet = reinterpret_cast<packet_type*>(data);
    switch (*packet) {
    case packet_type::change_direction_request: {
        if (size != sizeof(change_direction_request))
            return;
        auto* req = reinterpret_cast<change_direction_request*>(packet);
        auto player = find_player(req->player_id);
        if (player && player->socket_ == socket) {
            player->dir = req->dir;

            change_direction_response res;
            res.dir = player->dir;
            res.pos = player->pos;
            res.player_id = player->id;
            res.type = packet_type::change_direction_response;
            send_to_all(reinterpret_cast<char*>(&res), sizeof(res));
        }
    } break;
    case packet_type::shoot_request: {
        if (size != sizeof(shoot_request))
            return;
        auto* req = reinterpret_cast<shoot_request*>(packet);
        auto player = find_player(req->player_id);
        if (player && player->socket_ == socket) {
            auto bullet = new scene_object;
            bullet->id = bullet_id_counter_++;
            bullet->dir = vector2(0, -1);
            bullet->size = vector2(24, 8);
            bullet->pos = player->pos + vector2(28, -4);
            bullet->speed = 300;
            bullets_.push_back(bullet);

            shoot_response res;
            res.type = packet_type::shoot_response;
            res.bullet_id = bullet->id;
            res.pos = bullet->pos;
            send_to_all(reinterpret_cast<char*>(&res), sizeof(res));
        }
    } break;
    }
}

void multi_shoot::on_leave(SOCKET socket) {
    fprintf(stdout, "[%d] leaved! \n", static_cast<int>(socket));
    for (auto it = players_.begin(); it != players_.end(); ++it) {
        if ((*it)->socket_ == socket) {
            player_hit_response res;
            res.player_id = (*it)->id;
            res.player_health = 0;
            res.monster_id = -1;
            res.type = packet_type::player_hit_response;

            delete (*it);
            players_.erase(it);

            send_to_all(reinterpret_cast<char*>(&res), sizeof(player_hit_response));
            break;
        }
    }
}

void multi_shoot::spawn_enemy() {
    for (int i = 0; i < 5; ++i) {
        auto enemy = new scene_object;
        enemy->id = enemy_id_counter_++;
        enemy->dir = vector2(0, 1);
        enemy->hp = 5;
        enemy->size = vector2(43, 51);
        enemy->pos = vector2(120.f * i + 60.f - 21.5f, 0);
        enemy->speed = 200;
        enemies_.push_back(enemy);

        monster_spawn_response res;
        res.monster_id = enemy->id;
        res.pos = enemy->pos;
        res.hp = enemy->hp;
        res.type = packet_type::monster_spawn_response;
        send_to_all(reinterpret_cast<char*>(&res), sizeof(res));
    }
}

void multi_shoot::update_position(float dt) {
    for (auto it = players_.begin(); it != players_.end(); ++it) {
        (*it)->pos = (*it)->pos + (*it)->dir * (*it)->speed * dt;
    }

    for (auto it = bullets_.begin(); it != bullets_.end();) {
        (*it)->pos = (*it)->pos + (*it)->dir * (*it)->speed * dt;
        if ((*it)->pos.y < -25) {
            delete (*it);
            it = bullets_.erase(it);
        } else {
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

bool multi_shoot::player_enemy_collision(scene_object& enemy) {
    rect er(enemy.pos, enemy.size);
    for (auto it = players_.begin(); it != players_.end();) {
        rect br((*it)->pos, (*it)->size);
        if (tvdr::rect::is_overlapped(er, br)) {
            player_hit_response res;
            res.player_id = (*it)->id;
            res.player_health = --(*it)->hp;
            res.monster_id = enemy.id;
            res.type = packet_type::player_hit_response;

            send_to_all(reinterpret_cast<char*>(&res), sizeof(player_hit_response));

            auto player = *it;
            if (player->hp <= 0) {
                game_end_response gres;
                gres.player_id = player->id;
                gres.best_score = score_;
                gres.score = score_ - player->score;
                gres.type = packet_type::game_end_response;
                send_data(player->socket_, reinterpret_cast<char*>(&gres),
                          sizeof(game_end_response));

                players_.erase(it);
                delete player;
            }

            return true;
        } else
            ++it;
    }
    return false;
}

bool multi_shoot::bullet_enemy_collision(scene_object& enemy) {
    rect er(enemy.pos, enemy.size);
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        rect br((*it)->pos, (*it)->size);
        if (tvdr::rect::is_overlapped(er, br)) {
            monster_hit_response res;
            res.bullet_id = (*it)->id;
            res.monster_health = --enemy.hp;
            res.monster_id = enemy.id;
            res.type = packet_type::monster_hit_response;

            send_to_all(reinterpret_cast<char*>(&res), sizeof(res));

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

scene_object* multi_shoot::find_player(uint32_t id) {
    for (auto p : players_)
        if (p->id == id)
            return p;
    return nullptr;
}

void multi_shoot::send_to_all(char* data, int size) {
    for (auto p : players_) {
        this->send_data(p->socket_, data, size);
    }
}

void multi_shoot::on_update(float dt) {
    if (enemy_spawn_counter_ >= enemy_spawn_time_) {
        this->spawn_enemy();
        enemy_spawn_counter_ = 0;
    }
    this->update_position(dt);

    enemy_spawn_counter_ += dt;
}

void multi_shoot::first_data(SOCKET socket) {
    for (auto p : players_) {
        player_spawn_response res;
        res.hp = p->hp;
        res.player_id = p->id;
        res.pos = p->pos;
        res.dir = p->dir;
        res.type = packet_type::player_spawn_response;
        send_data(socket, reinterpret_cast<char*>(&res), sizeof(res));
    }
    for (auto e : enemies_) {
        monster_spawn_response res;
        res.hp = e->hp;
        res.monster_id = e->id;
        res.pos = e->pos;
        res.type = packet_type::monster_spawn_response;
        send_data(socket, reinterpret_cast<char*>(&res), sizeof(res));
    }
    for (auto b : bullets_) {
        shoot_response res;
        res.bullet_id = b->id;
        res.pos = b->pos;
        res.type = packet_type::shoot_response;
        send_data(socket, reinterpret_cast<char*>(&res), sizeof(res));
    }
}
