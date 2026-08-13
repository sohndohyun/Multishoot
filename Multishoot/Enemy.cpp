#include "Enemy.hpp"

using tvdr::game_manager;
using tvdr::graphics;
using tvdr::vector2;

enemy::enemy(Uint32 id, vector2 pos, Uint32 hp) : game_object("enemy.bmp") {
    set_scale(0.2f, -0.2f);
    health_bar_ = new game_object("1.bmp");
    health_bar_->set_color(255, 0, 0);
    health_bar_->set_local_position(0, -11);
    add_child(health_bar_);

    move_speed_ = 200.f;
    initialize(id, pos, hp);
}

void enemy::initialize(Uint32 id, vector2 pos, Uint32 hp) {
    this->id = id;
    set_position(pos);
    health_ = hp;
    update_health_bar();
}

bool enemy::set_health(Uint32 hp) {
    health_ = hp;
    update_health_bar();
    if (health_ <= 0)
        return true;
    return false;
}

void enemy::update() {
    auto pos = position();
    pos.y += move_speed_ * game_manager::delta_time();
    set_position(pos);

    if (pos.y > graphics::screen_height())
        set_active(false);
}

void enemy::update_health_bar() {
    auto size = print_size();
    health_bar_->set_scale(size.x * static_cast<float>(health_) / 5, 10);
}
