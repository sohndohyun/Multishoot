#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <atomic>
#include <cstddef>

namespace dr {

// A lock-free object cache backed by the Windows SLIST free-list.
// The pool must outlive all objects acquired from it and must not be destroyed
// while another thread is using acquire() or release().
template <class T> class object_pool final {
  private:
    struct alignas(MEMORY_ALLOCATION_ALIGNMENT) free_node {
        SLIST_ENTRY entry;
        T* value;
    };

  public:
    object_pool() noexcept {
        InitializeSListHead(&head_);
        cached_count_.store(0, std::memory_order_relaxed);
    }

    object_pool(const object_pool&) = delete;
    object_pool& operator=(const object_pool&) = delete;
    object_pool(object_pool&&) = delete;
    object_pool& operator=(object_pool&&) = delete;

    ~object_pool() noexcept {
        while (auto* entry = InterlockedPopEntrySList(&head_)) {
            auto* node = reinterpret_cast<free_node*>(entry);
            delete node->value;
            delete node;
        }
    }

    [[nodiscard]] T* acquire() {
        auto* entry = InterlockedPopEntrySList(&head_);
        if (entry == nullptr) {
            return new T;
        }

        auto* node = reinterpret_cast<free_node*>(entry);
        T* value = node->value;
        delete node;
        cached_count_.fetch_sub(1, std::memory_order_relaxed);
        return value;
    }

    void release(T* value) {
        if (value == nullptr) {
            return;
        }

        auto* node = new free_node{};
        node->value = value;
        cached_count_.fetch_add(1, std::memory_order_relaxed);
        InterlockedPushEntrySList(&head_, &node->entry);
    }

    [[nodiscard]] std::size_t cached_count() const noexcept {
        return cached_count_.load(std::memory_order_relaxed);
    }

  private:
    SLIST_HEADER head_{};
    std::atomic<std::size_t> cached_count_{0};
};

} // namespace dr
