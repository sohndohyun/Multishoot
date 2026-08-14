#pragma once

#include "engine/game_object.hpp"
#include "SDL_ttf.h"

#include <string>

class text final : public game_object {
  private:
    std::string text_;

    unsigned int size_;
    TTF_Font* font_;

    void initialize_texture();

  public:
    explicit text(std::string font_path, int size = 10);

    void set_text(std::string text);

    ~text() override;
};
