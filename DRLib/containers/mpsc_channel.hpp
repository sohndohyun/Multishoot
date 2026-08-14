#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>

namespace dr {

// Lock-free multi-producer, single-consumer channel.
// The channel must have exactly one receiving thread. Producers may call send()
// concurrently. close() prevents new sends and allows the consumer to drain
// messages already published before observing an empty channel.
template <class T> class mpsc_channel final {
  private:
    struct node {
        std::optional<T> value;
        std::atomic<node*> next{nullptr};
        std::atomic<bool> published{false};

        node() noexcept = default;

        template <class... Args>
        explicit node(std::in_place_t, Args&&... args)
            : value(std::in_place, std::forward<Args>(args)...) {}
    };

  public:
    mpsc_channel() : head_(new node), tail_(head_) {}

    mpsc_channel(const mpsc_channel&) = delete;
    mpsc_channel& operator=(const mpsc_channel&) = delete;
    mpsc_channel(mpsc_channel&&) = delete;
    mpsc_channel& operator=(mpsc_channel&&) = delete;

    ~mpsc_channel() {
        close();
        while (head_ != nullptr) {
            node* next = head_->next.load(std::memory_order_acquire);
            delete head_;
            head_ = next;
        }
    }

    [[nodiscard]] bool send(const T& value) {
        return emplace(value);
    }

    [[nodiscard]] bool send(T&& value) {
        return emplace(std::move(value));
    }

    template <class... Args> [[nodiscard]] bool emplace(Args&&... args) {
        if (!begin_send()) {
            return false;
        }

        node* entry = nullptr;
        try {
            entry = new node(std::in_place, std::forward<Args>(args)...);
        } catch (...) {
            finish_send(true);
            throw;
        }

        node* previous = tail_.exchange(entry, std::memory_order_acq_rel);
        previous->next.store(entry, std::memory_order_release);
        pending_.fetch_add(1, std::memory_order_release);
        entry->published.store(true, std::memory_order_release);
        finish_send(false);
        generation_.fetch_add(1, std::memory_order_release);
        generation_.notify_one();
        return true;
    }

    [[nodiscard]] bool try_receive(T& value) {
        auto received = try_receive_value();
        if (!received.has_value()) {
            return false;
        }

        value = std::move(*received);
        return true;
    }

    [[nodiscard]] std::optional<T> wait_receive(std::stop_token stop_token = {}) {
        assert_consumer();
        std::stop_callback wake_callback(stop_token, [this] {
            generation_.fetch_add(1, std::memory_order_release);
            generation_.notify_all();
        });

        for (;;) {
            if (stop_token.stop_requested()) {
                return std::nullopt;
            }

            if (auto value = try_receive_value(); value.has_value()) {
                return value;
            }

            if (is_closed() && pending() == 0 && active_senders() == 0) {
                return std::nullopt;
            }

            const auto generation = generation_.load(std::memory_order_acquire);
            generation_.wait(generation, std::memory_order_acquire);
        }
    }

    void close() noexcept {
        state_.fetch_or(closed_bit, std::memory_order_acq_rel);
        generation_.fetch_add(1, std::memory_order_release);
        generation_.notify_all();
    }

    [[nodiscard]] bool is_closed() const noexcept {
        return (state_.load(std::memory_order_acquire) & closed_bit) != 0;
    }

    [[nodiscard]] std::size_t pending() const noexcept {
        return pending_.load(std::memory_order_acquire);
    }

  private:
    [[nodiscard]] std::optional<T> try_receive_value() {
        assert_consumer();
        node* next = head_->next.load(std::memory_order_acquire);
        if (next == nullptr || !next->published.load(std::memory_order_acquire) ||
            !next->value.has_value()) {
            return std::nullopt;
        }

        std::optional<T> value(std::in_place, std::move(*next->value));
        next->value.reset();
        delete head_;
        head_ = next;
        pending_.fetch_sub(1, std::memory_order_release);
        return value;
    }

    [[nodiscard]] bool begin_send() noexcept {
        auto state = state_.load(std::memory_order_acquire);
        for (;;) {
            if ((state & closed_bit) != 0) {
                return false;
            }

            if (state_.compare_exchange_weak(state, state + 1, std::memory_order_acq_rel,
                                             std::memory_order_acquire)) {
                return true;
            }
        }
    }

    void finish_send(bool notify) noexcept {
        state_.fetch_sub(1, std::memory_order_release);
        if (notify) {
            generation_.fetch_add(1, std::memory_order_release);
            generation_.notify_all();
        }
    }

    [[nodiscard]] std::uint64_t active_senders() const noexcept {
        return state_.load(std::memory_order_acquire) & ~closed_bit;
    }

    void assert_consumer() {
        const auto current =
            static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
            1;
        std::uint64_t expected = 0;
        if (consumer_id_.compare_exchange_strong(expected, current, std::memory_order_acq_rel)) {
            return;
        }
        assert(expected == current && "mpsc_channel has more than one consumer");
    }

    node* head_;
    std::atomic<node*> tail_;
    std::atomic<std::size_t> pending_{0};
    static constexpr std::uint64_t closed_bit = std::uint64_t{1} << 63;

    std::atomic<std::uint64_t> state_{0};
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<std::uint64_t> consumer_id_{0};
};

} // namespace dr
