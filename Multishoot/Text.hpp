#pragma once

#include "GameObject.hpp"
#include "SDL_ttf.h"

#include <string>

namespace tvdr {
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
} // namespace tvdr
