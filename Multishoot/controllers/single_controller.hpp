#pragma once
#include "controllers/game_controller.hpp"
#include "math/vector.hpp"

#include <list>

struct scene_object {
    Uint32 id;
    dr::vector2 dir;
    dr::vector2 size;
    dr::vector2 pos;
    Uint32 speed;
    Uint32 hp;
    scene_object() : id(0), speed(0), hp(0) {}
};

class single_controller final : public game_controller {
  private:
    scene_object player_;
    std::list<scene_object*> enemies_;
    std::list<scene_object*> bullets_;

    float enemy_spawn_time_;
    float enemy_spawn_counter_;

    Uint32 enemy_id_counter_;
    Uint32 bullet_id_counter_;

    Uint32 score_;
    Uint32 best_score_;

    void spawn_enemy();
    void update_position();
    bool player_enemy_collision(scene_object& enemy);
    bool bullet_enemy_collision(scene_object& enemy);

  protected:
    void update() override;

  public:
    single_controller();
    ~single_controller() override;

    void change_direction(dr::vector2 dir, Uint32 id) override;
    void shoot(Uint32 id) override;
};
