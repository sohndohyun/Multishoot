#include "DRPacket.h"

#include <cstring>

dr_packet::dr_packet() = default;

void dr_packet::init() {
    std::memset(storage_.data(), 0, storage_.size());
    header_ = reinterpret_cast<frame_header*>(storage_.data());
    put_position_ = call_position_ = storage_.data() + sizeof(frame_header);
}

bool dr_packet::put(char* data, int size) {
    if (data == nullptr || size < 0 || put_position_ - storage_.data() + size > packet_capacity) {
        return false;
    }

    std::memcpy(put_position_, data, static_cast<std::size_t>(size));
    put_position_ += size;
    return true;
}

int dr_packet::call_packet(char* data) {
    const int packet_size = full_size();
    std::memcpy(data, storage_.data(), static_cast<std::size_t>(packet_size));
    return packet_size;
}

bool dr_packet::put_packet(char* data, int size) {
    if (data == nullptr || size < sizeof(frame_header) || size > packet_capacity) {
        return false;
    }

    std::memcpy(storage_.data(), data, static_cast<std::size_t>(size));
    header_ = reinterpret_cast<frame_header*>(storage_.data());
    put_position_ = storage_.data() + size;
    call_position_ = storage_.data() + sizeof(frame_header);
    return true;
}

bool dr_packet::call(char* data, int size) {
    if (data == nullptr || size < 0 || put_position_ - call_position_ < size) {
        return false;
    }

    std::memcpy(data, call_position_, static_cast<std::size_t>(size));
    call_position_ += size;
    return true;
}

dr_packet::frame_header* dr_packet::header() {
    return header_;
}

int dr_packet::size() const {
    return static_cast<int>(put_position_ - call_position_);
}

bool dr_packet::move_put_pointer(int size) {
    if (size < header_size() || size > packet_capacity) {
        return false;
    }

    put_position_ = storage_.data() + size;
    return true;
}
