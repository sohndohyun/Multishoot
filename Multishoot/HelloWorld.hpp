#pragma once

#include "Bullet.hpp"
#include "Enemy.hpp"
#include "GameController.h"
#include "LobbyScene.hpp"
#include "Player.hpp"
#include "TVDR.hpp"

#include <list>

class hello_world final : public tvdr::scene {
  protected:
    void start();
    void update() override;

    bullet* find_inactive_bullet();
    enemy* find_inactive_enemy();

    void spawn_enemy(Uint32 id, tvdr::vector2 pos, Uint32 hp);
    void spawn_bullet(Uint32 id, tvdr::vector2 pos);
    void spawn_player(Uint32 id, tvdr::vector2 pos, Uint32 hp, tvdr::vector2 dir);

    player* find_player(Uint32 id);
    bullet* find_bullet(Uint32 id);
    enemy* find_enemy(Uint32 id);

  public:
    hello_world(game_mode mode = game_mode::single);
    ~hello_world() override = default;

  private:
    game_controller* controller_;

    std::list<player*> player_list_;
    std::list<bullet*> bullet_list_;
    std::list<enemy*> enemy_list_;

    tvdr::text* kill_count_text_;

    Uint32 id_;
    tvdr::vector2 last_direction_;
    Uint32 kill_count_;

    float bullet_cool_time_;
    float bullet_cool_counter_;
};
