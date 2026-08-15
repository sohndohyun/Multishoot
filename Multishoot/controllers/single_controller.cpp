#include "controllers/single_controller.hpp"

#include "engine/game_manager.hpp"

#include <utility>

single_controller::single_controller() : player_id_(game_.add_player()) {
    flush_events();
}

void single_controller::update() {
    game_.update(game_manager::delta_time());
    flush_events();
}

void single_controller::change_direction(dr::vector2 direction) {
    game_.change_direction(player_id_, direction);
    flush_events();
}

void single_controller::shoot() {
    game_.shoot(player_id_);
    flush_events();
}

void single_controller::flush_events() {
    for (auto& event : game_.take_events())
        enqueue(std::move(event.packet));
}
