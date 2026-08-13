#pragma once

#include "Rect.hpp"
#include "SDL.h"

#include <string>

namespace tvdr {

class graphics {
  private:
    static graphics* instance_;
    static bool initialized_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;

  public:
    static graphics* instance();
    static graphics* instance(vector2 screen_size, const std::string& window_name);
    static void release();
    static bool is_initialized();

    void clear();
    void render();

    static vector2 screen_size() {
        return instance_->screen_size_;
    }
    static float screen_width() {
        return instance_->screen_size_.x;
    }
    static float screen_height() {
        return instance_->screen_size_.y;
    }
    static rect screen_rect() {
        return rect(0, 0, instance_->screen_size_.x, instance_->screen_size_.y);
    }
    static SDL_Texture* texture_from_surface(SDL_Surface* surface);
    static void render_texture(SDL_Texture* texture, SDL_Rect* destination, float rotation);

  private:
    graphics();
    graphics(vector2 screen_size, const std::string& window_name);
    ~graphics();

    bool initialize();
    vector2 screen_size_;
    std::string window_name_;
};
} // namespace tvdr
