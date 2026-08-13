#include "Graphics.hpp"

#include "SDL_ttf.h"

namespace tvdr {

graphics* graphics::instance_ = nullptr;
bool graphics::initialized_ = false;

graphics* graphics::instance() {
    if (instance_ == nullptr)
        instance_ = new graphics();
    return instance_;
}

graphics* graphics::instance(vector2 screen_size, const std::string& window_name) {
    if (instance_ == nullptr)
        instance_ = new graphics(screen_size, window_name);
    return instance_;
}

void graphics::release() {
    delete instance_;
    instance_ = nullptr;

    initialized_ = false;
}

bool graphics::is_initialized() {
    return initialized_;
}

graphics::graphics() {
    screen_size_ = vector2(800, 600);
    window_name_ = "tvdr Project";
    renderer_ = nullptr;
    initialized_ = initialize();
}

graphics::graphics(vector2 screen_size, const std::string& window_name) {
    screen_size_ = screen_size;
    window_name_ = window_name;
    renderer_ = nullptr;
    initialized_ = initialize();
}

graphics::~graphics() {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    SDL_DestroyRenderer(renderer_);
    TTF_Quit();
    SDL_Quit();
}

bool graphics::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL InitialIzetion Error:%s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        printf("SDL_TTF InitialIzetion Error:%s\n", TTF_GetError());
        return false;
    }

    window_ = SDL_CreateWindow(window_name_.c_str(), SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, static_cast<int>(screen_size_.x),
                               static_cast<int>(screen_size_.y), SDL_WINDOW_SHOWN);

    if (window_ == nullptr) {
        printf("window creation ERROR : %s\n", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, 0);
    if (renderer_ == nullptr) {
        printf("renderer creation ERROR : %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void graphics::clear() {
    SDL_RenderClear(renderer_);
}

void graphics::render() {
    SDL_RenderPresent(renderer_);
    SDL_UpdateWindowSurface(window_);
}

SDL_Texture* graphics::texture_from_surface(SDL_Surface* surface) {
    return SDL_CreateTextureFromSurface(instance()->renderer_, surface);
}

void graphics::render_texture(SDL_Texture* texture, SDL_Rect* destination, float rotation) {
    auto renderer = instance()->renderer_;
    int flip = SDL_FLIP_NONE;
    if (destination->h < 0) {
        flip |= SDL_FLIP_VERTICAL;
        destination->h = -destination->h;
    }
    if (destination->w < 0) {
        flip |= SDL_FLIP_HORIZONTAL;
        destination->w = -destination->w;
    }
    SDL_RenderCopyEx(renderer, texture, nullptr, destination, rotation, nullptr,
                     static_cast<SDL_RendererFlip>(flip));
}
} // namespace tvdr
