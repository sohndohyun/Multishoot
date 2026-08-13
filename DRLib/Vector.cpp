#include "Vector.hpp"

#include <cmath>

namespace tvdr {

vector2 vector2::norm() noexcept {
    const float length = std::sqrt(x * x + y * y);
    if (length > 0) {
        x /= length;
        y /= length;
    }
    return *this;
}

} // namespace tvdr
