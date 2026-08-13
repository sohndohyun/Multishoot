#pragma once

#include "TVDR.hpp"

enum class game_mode { single, multi };

class lobby_scene final : public tvdr::scene {
  protected:
    void update() override;
    void start();

    int best_score();

    tvdr::text* single_;
    tvdr::text* multi_;
    tvdr::game_object* selected_text_;
    game_mode mode_;
    void change_mode();

  public:
    lobby_scene();
    ~lobby_scene() override = default;
};
