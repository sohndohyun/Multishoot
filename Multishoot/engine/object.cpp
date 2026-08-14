#include "engine/object.hpp"

#include "engine/game_manager.hpp"

#include <algorithm>

using dr::vector2;

object::~object() {
    if (parent_)
        parent_->children_.remove(this);
    while (!children_.empty())
        delete children_.front();
}

bool object::add_child(object* obj) {
    if (std::find(children_.begin(), children_.end(), obj) == children_.end()) {
        children_.push_back(obj);
        obj->parent_ = this;
        obj->set_position(obj->local_position());
        return true;
    }
    return false;
}

void object::update() {}
void object::render() {}

void object::set_parent(object* parent) {
    if (parent_)
        parent_->children_.remove(this);
    parent_ = parent;
    parent->add_child(this);
}

const object* object::parent() const {
    return parent_;
}

void object::set_active(bool active) {
    active_ = active;
}

bool object::is_active() const {
    return active_;
}

void object::release() {
    needs_release_ = true;
}

vector2 object::position() const {
    if (!parent_)
        return position_;
    return parent_->position() + position_;
}

void object::set_position(float x, float y) {
    if (!parent_)
        set_local_position(x, y);
    else
        set_local_position(vector2(x, y) - parent_->position());
}

void object::set_position(vector2 vec) {
    if (!parent_)
        set_local_position(vec);
    else
        set_local_position(vec - parent_->position());
}
