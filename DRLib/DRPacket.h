#pragma once

#include <array>
#include <cstddef>

inline constexpr std::size_t packet_capacity = 512;

class dr_packet {
  public:
    struct frame_header {
        char code;
        int size;
    };

    static constexpr char code = 12;
    static constexpr int header_size() noexcept {
        return sizeof(frame_header);
    }
    static constexpr int max_data_size() noexcept {
        return static_cast<int>(packet_capacity - sizeof(frame_header));
    }

    dr_packet();

    void init();
    bool put(char* data, int size);
    int call_packet(char* data);
    bool put_packet(char* data, int size);
    bool call(char* data, int size);
    frame_header* header();
    int size() const;
    int full_size() const {
        return size() + sizeof(frame_header);
    }
    char* buffer() {
        return storage_.data();
    }
    char* call_pointer() {
        return call_position_;
    }
    bool move_put_pointer(int size);

  private:
    alignas(std::max_align_t) std::array<char, packet_capacity> storage_{};
    frame_header* header_{};
    char* put_position_{};
    char* call_position_{};
};

static_assert(sizeof(dr_packet::frame_header) == 8);
