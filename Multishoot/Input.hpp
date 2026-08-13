#pragma once

#include "SDL.h"

namespace tvdr {

class input final {
  private:
    static input* instance_;
    bool current_state_[SDL_NUM_SCANCODES];
    bool previous_state_[SDL_NUM_SCANCODES];

  public:
    static input* instance();
    static void release();

    static bool is_key_pressed(SDL_Scancode key_code);
    static bool is_key_down(SDL_Scancode key_code);
    static bool is_key_up(SDL_Scancode key_code);

    void update_key_state();

  private:
    static void key_state(SDL_Scancode scan_code, bool& current_state, bool& previous_state);
    input();
    ~input() = default;
};
} // namespace tvdr
