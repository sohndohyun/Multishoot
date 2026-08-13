#pragma once

#include "Vector.hpp"

#include <list>
#include <string>
#include <vector>

namespace tvdr {

class object {
    friend class scene;
    friend class game_manager;

  private:
    std::list<object*> children_;
    object* parent_ = nullptr;
    bool needs_release_ = false;
    bool active_ = true;

  protected:
    vector2 position_;
    vector2 scale_;
    float rotation_ = 0;

    bool add_child(object* obj);

    virtual void update();
    virtual void render();

  public:
    void set_parent(object* parent);
    [[nodiscard]] const object* parent() const;

    void set_active(bool active);
    [[nodiscard]] bool is_active() const;

    void release();

    vector2 position() const;
    vector2 local_position() const {
        return position_;
    }
    void set_local_position(float x, float y) {
        position_.x = x;
        position_.y = y;
    }
    void set_local_position(vector2 vec) {
        position_ = vec;
    }
    void set_position(float x, float y);
    void set_position(vector2 vec);

    void set_scale(float x, float y) {
        scale_.x = x;
        scale_.y = y;
    }
    void set_scale(vector2 vec) {
        scale_ = vec;
    }
    [[nodiscard]] const vector2& scale() const {
        return scale_;
    }

    object() = default;
    virtual ~object();
};
} // namespace tvdr
