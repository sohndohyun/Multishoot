#include "math/vector.hpp"

#include <cmath>

namespace dr {

vector2 vector2::norm() noexcept {
    const float length = std::sqrt(x * x + y * y);
    if (length > 0) {
        x /= length;
        y /= length;
    }
    return *this;
}

} // namespace dr
