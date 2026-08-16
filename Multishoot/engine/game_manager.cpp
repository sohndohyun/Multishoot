#include "engine/game_manager.hpp"

#include <Windows.h>

using dr::vector2;

inline constexpr DWORD frame_duration_ms = 10;

game_manager* game_manager::instance_ = nullptr;
game_manager* game_manager::instance() {
    if (instance_ == nullptr)
        instance_ = new game_manager();
    return instance_;
}

game_manager* game_manager::instance(vector2 screen_size, const std::string& window_name) {
    if (instance_ == nullptr)
        instance_ = new game_manager(screen_size, window_name);
    return instance_;
}

void game_manager::release() {
    delete instance_;
    instance_ = nullptr;
}

game_manager::game_manager() {
    quit_ = false;
    graphics_ = graphics::instance();
    save_data_ = player_pref::instance();
    current_scene_ = nullptr;
    next_scene_ = nullptr;
    input_ = input::instance();

    current_time_ = 0;
    start_time_ = 0;
    last_time_ = 0;
    if (!graphics::is_initialized())
        quit_ = true;
}

game_manager::game_manager(vector2 screen_size, const std::string& window_name) {
    quit_ = false;
    graphics_ = graphics::instance(screen_size, window_name);
    save_data_ = player_pref::instance();
    current_scene_ = nullptr;
    next_scene_ = nullptr;
    input_ = input::instance();

    current_time_ = 0;
    start_time_ = 0;
    last_time_ = 0;
    if (!graphics::is_initialized())
        quit_ = true;
}

game_manager::~game_manager() {
    if (current_scene_)
        delete current_scene_;
    if (next_scene_)
        delete next_scene_;
    graphics::release();
    player_pref::release();
    input::release();
    graphics_ = nullptr;
    save_data_ = nullptr;
    input_ = nullptr;
}

bool game_manager::change_scene(scene* next_scene) {
    auto gm = instance();
    if (gm->next_scene_)
        return false;
    gm->next_scene_ = next_scene;
    return true;
}

void game_manager::initialize(vector2 screen_size, const std::string& window_name) {
    instance(screen_size, window_name);
}

int game_manager::run(scene* initial_scene) {
    if (!initial_scene)
        return 1;
    instance()->start_time_ = SDL_GetTicks64();
    instance()->current_scene_ = initial_scene;
    instance()->main_loop();
    game_manager::release();
    return 0;
}

void game_manager::main_loop() {
    if (!current_scene_)
        return;

    current_time_ = last_time_ = start_time_;
    SDL_Event ev;
    while (!quit_) {
        current_time_ = SDL_GetTicks64();
        graphics_->clear();
        input_->begin_frame();
        while (SDL_PollEvent(&ev) != 0) {
            switch (ev.type) {
            case SDL_QUIT:
                quit_ = true;
                break;
            default:
                input_->handle_event(ev);
                break;
            }
        }

        input_->update_key_state();

        if (next_scene_) {
            delete current_scene_;
            current_scene_ = next_scene_;
            next_scene_ = nullptr;
        }
        current_scene_->update_all();
        graphics_->render();
        current_scene_->release_all();
        const auto frame_end = SDL_GetTicks64();
        const auto elapsed = frame_end - current_time_;
        if (elapsed < frame_duration_ms)
            Sleep(static_cast<DWORD>(frame_duration_ms - elapsed));
        last_time_ = current_time_;
    }
}

long long game_manager::time() {
    return instance()->current_time_ - instance()->start_time_;
}

float game_manager::delta_time() {
    const auto elapsed = instance()->current_time_ - instance()->last_time_;
    return static_cast<float>(elapsed) / 1000.f;
}

bool game_manager::add_object(object* obj) {
    if (instance_->next_scene_)
        return instance_->next_scene_->add_child(obj);
    if (instance_->current_scene_)
        return instance_->current_scene_->add_child(obj);
    return false;
}
