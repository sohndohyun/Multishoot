#pragma once

#include "engine/object.hpp"
#include "math/rect.hpp"
#include <SDL2/SDL.h>
#include "math/vector.hpp"

#include <string>

class game_object : public object {
    friend class text;

  private:
    SDL_Surface* surface_;
    SDL_Texture* texture_;
    dr::vector2 texture_size_;
    dr::vector2 anchor_;
    SDL_Color color_;

    void render() override;

  public:
    dr::vector2 const& texture_size() const {
        return texture_size_;
    }
    dr::vector2 print_size() const {
        return texture_size_ * scale_.abs();
    }
    void set_color(Uint8 r, Uint8 g, Uint8 b);
    dr::vector2 const& anchor() const {
        return anchor_;
    }
    void set_anchor(dr::vector2 const& anchor) {
        anchor_ = anchor;
    }
    dr::rect bounds() const {
        return dr::rect(position(), print_size());
    }

    explicit game_object(std::string image_path);
    ~game_object() override;

  protected:
    game_object();
};
