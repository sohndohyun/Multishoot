#pragma once

#include "engine/game_object.hpp"
#include "engine/scene.hpp"
#include "engine/text.hpp"

enum class game_mode { single, multi };

class lobby_scene final : public scene {
  protected:
    void update() override;
    void start();

    int best_score();

    text* single_;
    text* multi_;
    game_object* selected_text_;
    game_mode mode_;
    void change_mode();

  public:
    lobby_scene();
    ~lobby_scene() override = default;
};
