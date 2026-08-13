#include "Input.hpp"

#include <Windows.h>

namespace tvdr {
input* input::instance_ = nullptr;
input* input::instance() {
    if (instance_ == nullptr)
        instance_ = new input();
    return instance_;
}

void input::release() {
    delete instance_;
    instance_ = nullptr;
}

input::input() {
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        previous_state_[i] = false;
        current_state_[i] = false;
    }
}

bool input::is_key_down(SDL_Scancode key_code) {
    bool previous_state;
    bool current_state;
    key_state(key_code, current_state, previous_state);
    return !previous_state && current_state;
}

bool input::is_key_pressed(SDL_Scancode key_code) {
    bool previous_state;
    bool current_state;
    key_state(key_code, current_state, previous_state);
    return current_state;
}

bool input::is_key_up(SDL_Scancode key_code) {
    bool previous_state;
    bool current_state;
    key_state(key_code, current_state, previous_state);
    return previous_state && !current_state;
}

void input::update_key_state() {
    auto key_states = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        previous_state_[i] = current_state_[i];
        current_state_[i] = key_states[i] != 0;
    }
}

void input::key_state(SDL_Scancode scan_code, bool& current_state, bool& previous_state) {
    current_state = instance_->current_state_[scan_code];
    previous_state = instance_->previous_state_[scan_code];
}
} // namespace tvdr
