#pragma once
#include "engine/graphics.hpp"
#include "engine/input.hpp"
#include "engine/player_pref.hpp"
#include "engine/scene.hpp"

#include <list>

class game_manager {
    friend class object;

  private:
    static game_manager* instance_;
    input* input_;
    player_pref* save_data_;
    graphics* graphics_;

    bool quit_;
    scene* current_scene_;
    scene* next_scene_;

    Uint64 start_time_;
    Uint64 current_time_;
    Uint64 last_time_;

  public:
    static game_manager* instance();
    static game_manager* instance(dr::vector2 screen_size, const std::string& window_name);
    static void release();
    static bool change_scene(scene* next_scene);
    static void initialize(dr::vector2 screen_size, const std::string& window_name);
    static int run(scene* initial_scene);
    static long long time();
    static float delta_time();
    static bool add_object(object* obj);
    static void quit() {
        instance()->quit_ = true;
    }

  private:
    void main_loop();

    game_manager();
    game_manager(dr::vector2 screen_size, const std::string& window_name);
    ~game_manager();
};
