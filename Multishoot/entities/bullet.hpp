#pragma once
#include "engine/game_object.hpp"
#include "math/vector.hpp"

class bullet final : public game_object {
  public:
    bullet(Uint32 id, dr::vector2 start_position);
    ~bullet() override = default;

    void initialize(Uint32 id, dr::vector2 start_position);

    Uint32 id;

  protected:
    void update() override;

  private:
    dr::vector2 direction_;
    float move_speed_;
};
