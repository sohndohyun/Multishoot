#include "engine/text.hpp"

#include "engine/graphics.hpp"


text::text(std::string font_path, int size) : game_object() {
    font_ = TTF_OpenFont((std::string("resource/") + font_path).c_str(), size);
    size_ = size;
    text_ = " ";

    initialize_texture();
}

text::~text() {
    if (font_)
        TTF_CloseFont(font_);
}

void text::initialize_texture() {
    if (surface_)
        SDL_FreeSurface(surface_);
    if (texture_)
        SDL_DestroyTexture(texture_);

    surface_ = TTF_RenderText_Blended(font_, text_.c_str(), color_);
    if (surface_ == nullptr)
        return;

    texture_ = graphics::texture_from_surface(surface_);
    int w, h;
    SDL_QueryTexture(texture_, nullptr, nullptr, &w, &h);
    texture_size_.x = static_cast<float>(w);
    texture_size_.y = static_cast<float>(h);
}

void text::set_text(std::string text) {
    text_ = text;
    initialize_texture();
}
