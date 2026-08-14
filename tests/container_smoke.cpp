#include "containers/mpsc_channel.hpp"
#include "containers/object_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace {

struct tracked {
    static std::atomic<int> destructions;

    tracked() = default;
    tracked(const tracked&) = delete;
    tracked& operator=(const tracked&) = delete;
    ~tracked() {
        destructions.fetch_add(1, std::memory_order_relaxed);
    }
};

std::atomic<int> tracked::destructions{0};

struct move_only_message {
    int producer = 0;
    int sequence = 0;

    move_only_message(int producer_value, int sequence_value)
        : producer(producer_value), sequence(sequence_value) {}
    move_only_message(const move_only_message&) = delete;
    move_only_message& operator=(const move_only_message&) = delete;
    move_only_message(move_only_message&&) noexcept = default;
    move_only_message& operator=(move_only_message&&) noexcept = default;
};

void test_object_pool() {
    tracked::destructions.store(0, std::memory_order_relaxed);
    {
        dr::object_pool<tracked> pool;
        auto* first = pool.acquire();
        pool.release(first);
        assert(pool.cached_count() == 1);
        auto* reused = pool.acquire();
        assert(reused == first);
        pool.release(reused);

        std::vector<std::thread> workers;
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&pool] {
                for (int iteration = 0; iteration < 1000; ++iteration) {
                    auto* value = pool.acquire();
                    pool.release(value);
                }
            });
        }
        for (auto& worker : workers) {
            worker.join();
        }
        assert(pool.cached_count() > 0);
    }
    assert(tracked::destructions.load(std::memory_order_relaxed) > 0);
}

void test_mpsc_channel() {
    dr::mpsc_channel<move_only_message> channel;
    constexpr int producer_count = 4;
    constexpr int messages_per_producer = 500;
    std::vector<std::thread> producers;
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&channel, producer] {
            for (int sequence = 0; sequence < messages_per_producer; ++sequence) {
                assert(channel.emplace(producer, sequence));
            }
        });
    }

    int received = 0;
    std::vector<int> last_sequence(producer_count, -1);
    while (received < producer_count * messages_per_producer) {
        move_only_message message(0, 0);
        if (!channel.try_receive(message)) {
            std::this_thread::yield();
            continue;
        }
        assert(message.sequence == last_sequence[message.producer] + 1);
        last_sequence[message.producer] = message.sequence;
        ++received;
    }
    for (auto& producer : producers) {
        producer.join();
    }
    channel.close();
    assert(channel.is_closed());
    assert(channel.pending() == 0);
    assert(!channel.send(move_only_message(0, 0)));

    dr::mpsc_channel<int> waiting_channel;
    std::stop_source stop_source;
    std::atomic<bool> got_message{false};
    std::thread consumer([&] {
        auto value = waiting_channel.wait_receive(stop_source.get_token());
        got_message.store(value.has_value(), std::memory_order_release);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(waiting_channel.send(42));
    consumer.join();
    assert(got_message.load(std::memory_order_acquire));

    dr::mpsc_channel<int> stopped_channel;
    std::stop_source stopped_source;
    std::atomic<bool> stopped{false};
    std::thread stopped_consumer([&] {
        stopped.store(!stopped_channel.wait_receive(stopped_source.get_token()).has_value(),
                      std::memory_order_release);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    stopped_source.request_stop();
    stopped_consumer.join();
    assert(stopped.load(std::memory_order_acquire));

    dr::mpsc_channel<int> drain_channel;
    assert(drain_channel.send(1));
    assert(drain_channel.send(2));
    drain_channel.close();
    auto first = drain_channel.wait_receive();
    auto second = drain_channel.wait_receive();
    auto empty = drain_channel.wait_receive();
    assert(first && *first == 1);
    assert(second && *second == 2);
    assert(!empty);

    for (int iteration = 0; iteration < 1000; ++iteration) {
        dr::mpsc_channel<int> race_channel;
        std::atomic<bool> start{false};
        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();
            [[maybe_unused]] const bool accepted = race_channel.send(7);
        });
        start.store(true, std::memory_order_release);
        race_channel.close();
        producer.join();
        while (race_channel.wait_receive()) {
        }
    }
}

} // namespace

int main() {
    test_object_pool();
    test_mpsc_channel();
}
