#pragma once
#include "TVDR.hpp"

class bullet final : public tvdr::game_object {
  public:
    bullet(Uint32 id, tvdr::vector2 start_position);
    ~bullet() override = default;

    void initialize(Uint32 id, tvdr::vector2 start_position);

    Uint32 id;

  protected:
    void update() override;

  private:
    tvdr::vector2 direction_;
    float move_speed_;
};
