#pragma once

namespace tvdr {

class vector2 {
  public:
    float x{};
    float y{};

    constexpr vector2(float x_value = 0, float y_value = 0) noexcept : x(x_value), y(y_value) {}

    vector2 norm() noexcept;
    [[nodiscard]] constexpr vector2 abs() const noexcept {
        return {x < 0 ? -x : x, y < 0 ? -y : y};
    }

    [[nodiscard]] constexpr vector2 operator+(const vector2& value) const noexcept {
        return {x + value.x, y + value.y};
    }
    [[nodiscard]] constexpr vector2 operator-(const vector2& value) const noexcept {
        return {x - value.x, y - value.y};
    }
    [[nodiscard]] constexpr vector2 operator*(float value) const noexcept {
        return {x * value, y * value};
    }
    [[nodiscard]] constexpr vector2 operator*(const vector2& value) const noexcept {
        return {x * value.x, y * value.y};
    }
    [[nodiscard]] constexpr vector2 operator/(float value) const noexcept {
        return {x / value, y / value};
    }
    [[nodiscard]] constexpr bool operator!=(const vector2& value) const noexcept {
        return !(*this == value);
    }
    [[nodiscard]] constexpr bool operator==(const vector2& value) const noexcept {
        return x == value.x && y == value.y;
    }
};

} // namespace tvdr
