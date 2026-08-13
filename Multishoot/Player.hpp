#pragma once

#include "TVDR.hpp"

class player final : public tvdr::game_object {
  public:
    player(Uint32 id, tvdr::vector2 pos, Uint32 hp);
    ~player() override = default;

    void set_health(Uint32 hp);
    int health() {
        return health_;
    }
    void set_direction(tvdr::vector2 const& dir) {
        direction_ = dir;
    }
    tvdr::vector2 const& direction() {
        return direction_;
    }

  protected:
    void update() override;
    void update_health_bar();

  private:
    float move_speed_;
    tvdr::vector2 direction_;
    Uint32 health_;
    tvdr::game_object* health_bar_;

  public:
    Uint32 id;
};
