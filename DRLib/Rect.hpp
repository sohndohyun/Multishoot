#pragma once

#include "Vector.hpp"

namespace tvdr {

class rect {
  private:
    vector2 size_;

  public:
    vector2 pos;

    constexpr rect(float x = 0, float y = 0, float width = 0, float height = 0) noexcept
        : pos(x, y), size_(width < 0 ? 0 : width, height < 0 ? 0 : height) {}
    constexpr rect(vector2 position, vector2 size) noexcept
        : pos(position), size_(size.x < 0 ? 0 : size.x, size.y < 0 ? 0 : size.y) {}

    constexpr void set_size(vector2 size) noexcept {
        size_.x = size.x < 0 ? 0 : size.x;
        size_.y = size.y < 0 ? 0 : size.y;
    }
    [[nodiscard]] constexpr const vector2& size() const noexcept {
        return size_;
    }
    [[nodiscard]] static constexpr bool is_overlapped(const rect& first,
                                                      const rect& second) noexcept {
        return !(first.pos.x > second.pos.x + second.size_.x ||
                 second.pos.x > first.pos.x + first.size_.x ||
                 first.pos.y > second.pos.y + second.size_.y ||
                 second.pos.y > first.pos.y + first.size_.y);
    }
    [[nodiscard]] constexpr bool is_overlapped(const rect& other) const noexcept {
        return is_overlapped(*this, other);
    }
};

} // namespace tvdr
