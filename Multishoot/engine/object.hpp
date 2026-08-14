#pragma once

#include "math/vector.hpp"

#include <list>
#include <string>
#include <vector>


class object {
    friend class scene;
    friend class game_manager;

  private:
    std::list<object*> children_;
    object* parent_ = nullptr;
    bool needs_release_ = false;
    bool active_ = true;

  protected:
    dr::vector2 position_;
    dr::vector2 scale_;
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

    dr::vector2 position() const;
    dr::vector2 local_position() const {
        return position_;
    }
    void set_local_position(float x, float y) {
        position_.x = x;
        position_.y = y;
    }
    void set_local_position(dr::vector2 vec) {
        position_ = vec;
    }
    void set_position(float x, float y);
    void set_position(dr::vector2 vec);

    void set_scale(float x, float y) {
        scale_.x = x;
        scale_.y = y;
    }
    void set_scale(dr::vector2 vec) {
        scale_ = vec;
    }
    [[nodiscard]] const dr::vector2& scale() const {
        return scale_;
    }

    object() = default;
    virtual ~object();
};
