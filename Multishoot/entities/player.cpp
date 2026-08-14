#include "entities/player.hpp"

#include "entities/bullet.hpp"
#include "engine/game_manager.hpp"

using dr::vector2;

player::player(Uint32 id_value, vector2 pos, Uint32 hp) : game_object("player.bmp") {
    set_scale(0.2f, 0.2f);

    auto size = print_size();
    move_speed_ = 150.f;
    health_ = hp;

    direction_ = vector2(0, 0);
    id = id_value;

    health_bar_ = new game_object("1.bmp");
    health_bar_->set_color(255, 0, 0);
    health_bar_->set_local_position(0, size.y + 10);
    add_child(health_bar_);

    set_position(pos);

    update_health_bar();
}

void player::set_health(Uint32 hp) {
    health_ = hp;
    update_health_bar();
}

void player::update() {
    auto pos = position();

    set_position(pos + direction_ * move_speed_ * game_manager::delta_time());
}

void player::update_health_bar() {
    auto size = print_size();
    health_bar_->set_scale(size.x * static_cast<float>(health_) / 5, 10);
}
