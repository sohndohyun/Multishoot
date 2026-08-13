#pragma once

#include "Object.hpp"
#include "Rect.hpp"
#include "SDL.h"
#include "Vector.hpp"

#include <string>

namespace tvdr {
class game_object : public object {
    friend class text;

  private:
    SDL_Surface* surface_;
    SDL_Texture* texture_;
    vector2 texture_size_;
    vector2 anchor_;
    SDL_Color color_;

    void render() override;

  public:
    vector2 const& texture_size() const {
        return texture_size_;
    }
    vector2 print_size() const {
        return texture_size_ * scale_.abs();
    }
    void set_color(Uint8 r, Uint8 g, Uint8 b);
    vector2 const& anchor() const {
        return anchor_;
    }
    void set_anchor(vector2 const& anchor) {
        anchor_ = anchor;
    }
    tvdr::rect bounds() const {
        return tvdr::rect(position(), print_size());
    }

    explicit game_object(std::string image_path);
    ~game_object() override;

  protected:
    game_object();
};
} // namespace tvdr
