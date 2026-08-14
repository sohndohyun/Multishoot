#pragma once

#include "engine/game_object.hpp"
#include "math/vector.hpp"

class player final : public game_object {
  public:
    player(Uint32 id, dr::vector2 pos, Uint32 hp);
    ~player() override = default;

    void set_health(Uint32 hp);
    int health() {
        return health_;
    }
    void set_direction(dr::vector2 const& dir) {
        direction_ = dir;
    }
    dr::vector2 const& direction() {
        return direction_;
    }

  protected:
    void update() override;
    void update_health_bar();

  private:
    float move_speed_;
    dr::vector2 direction_;
    Uint32 health_;
    game_object* health_bar_;

  public:
    Uint32 id;
};
