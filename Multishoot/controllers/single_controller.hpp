#pragma once

#include "controllers/game_controller.hpp"
#include "game/game_simulation.hpp"

class single_controller final : public game_controller {
  protected:
    void update() override;

  public:
    single_controller();
    ~single_controller() override = default;

    void change_direction(dr::vector2 direction) override;
    void shoot() override;

  private:
    multishoot::game_simulation game_;
    multishoot::player_id player_id_;

    void flush_events();
};
