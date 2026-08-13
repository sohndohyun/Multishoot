#pragma once

#include "Object.hpp"

namespace tvdr {
class scene : public object {
    friend class game_manager;

  private:
    void update_all(object* obj = nullptr);
    void release_all(object* obj = nullptr);

  public:
    scene();
    ~scene() override = default;
};
} // namespace tvdr
