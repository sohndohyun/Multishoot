#include "entities/bullet.hpp"

#include "engine/game_manager.hpp"
#include "engine/graphics.hpp"
#include "game/game_simulation.hpp"

using dr::vector2;

bullet::bullet(Uint32 id, vector2 start_position) : game_object("1.bmp") {
    set_scale(24.f, 8.f);
    set_color(255, 255, 0);

    move_speed_ = multishoot::rules::bullet_speed;
    initialize(id, start_position);
}

void bullet::initialize(Uint32 id, dr::vector2 start_position) {
    this->id = id;
    direction_ = vector2(0, -1);
    set_position(start_position);
}

void bullet::update() {
    auto pos = position();
    set_position(pos + direction_ * move_speed_ * game_manager::delta_time());
    if (!bounds().is_overlapped(graphics::screen_rect()))
        set_active(false);
}
