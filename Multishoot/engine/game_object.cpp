#include "engine/game_object.hpp"

#include "engine/graphics.hpp"

#include <string>

using dr::vector2;

game_object::game_object(std::string image_path) : anchor_(0, 0) {
    scale_ = vector2(1, 1);

    surface_ = SDL_LoadBMP((std::string("resource/") + image_path).c_str());
    texture_ = graphics::texture_from_surface(surface_);
    int w, h;
    SDL_QueryTexture(texture_, nullptr, nullptr, &w, &h);
    texture_size_ = vector2(static_cast<float>(w), static_cast<float>(h));
    set_color(255, 255, 255);
}

game_object::game_object() : anchor_(0, 0) {
    scale_ = vector2(1, 1);
    surface_ = nullptr;
    texture_ = nullptr;
    texture_size_ = vector2(0, 0);
    set_color(255, 255, 255);
}

game_object::~game_object() {
    if (texture_)
        SDL_DestroyTexture(texture_);
    if (surface_)
        SDL_FreeSurface(surface_);
}

void game_object::render() {
    if (texture_ == nullptr)
        return;

    SDL_Rect rect;
    vector2 pos = position();

    rect.w = static_cast<int>(texture_size_.x * scale_.x);
    rect.h = static_cast<int>(texture_size_.y * scale_.y);

    rect.x = static_cast<int>(pos.x) - static_cast<int>(rect.w * anchor_.x);
    rect.y = static_cast<int>(pos.y) - static_cast<int>(rect.h * anchor_.y);

    graphics::render_texture(texture_, &rect, rotation_);
}

void game_object::set_color(Uint8 r, Uint8 g, Uint8 b) {
    color_ = {r, g, b, SDL_ALPHA_OPAQUE};
    SDL_SetTextureColorMod(texture_, r, g, b);
}
