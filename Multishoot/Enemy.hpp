#pragma once

#include "TVDR.hpp"

class enemy final : public tvdr::game_object {
  public:
    enemy(Uint32 id, tvdr::vector2 pos, Uint32 hp);
    ~enemy() override = default;
    void initialize(Uint32 id, tvdr::vector2 pos, Uint32 hp);

    bool set_health(Uint32 hp);

    Uint32 id;

  protected:
    void update() override;
    void update_health_bar();

  private:
    float move_speed_;

    Uint32 health_;
    tvdr::game_object* health_bar_;
};
