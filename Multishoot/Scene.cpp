#include "Scene.hpp"

namespace tvdr {

scene::scene() {}

void scene::update_all(object* obj) {
    if (obj == nullptr)
        obj = this;

    if (obj->is_active() == false)
        return;

    obj->update();
    if (obj->is_active() == false)
        return;
    obj->render();

    for (auto child : obj->children_) {
        update_all(child);
    }
}

void scene::release_all(object* obj) {
    if (obj == nullptr)
        obj = this;

    for (auto it = obj->children_.begin(); it != obj->children_.end();) {
        auto child = *it;
        if (child->needs_release_) {
            it = obj->children_.erase(it);
            delete child;
        } else {
            release_all(child);
            ++it;
        }
    }
}
} // namespace tvdr
