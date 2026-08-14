#pragma once

#include <WinSock2.h>
#include <cstdint>

namespace dr {

class ring_buffer final {
  public:
    enum class buffer_constant : std::uint16_t {
        buffer_default = 20960,
        buffer_blank = 8,
    };

    ring_buffer();
    explicit ring_buffer(int buffer_size);
    ~ring_buffer();

    void initialize(int buffer_size);
    [[nodiscard]] int buffer_size() const;
    [[nodiscard]] int used_size() const;
    [[nodiscard]] int free_size() const;
    [[nodiscard]] int contiguous_read_size() const;
    [[nodiscard]] int contiguous_write_size() const;
    int write(char* data, int size);
    int read(char* destination, int size);
    int peek(char* destination, int size) const;
    void remove_data(int size);
    void clear();
    char* buffer();
    char* read_buffer();
    char* write_buffer();
    void lock();
    void unlock();

  private:
    static constexpr int default_size = static_cast<int>(buffer_constant::buffer_default);
    static constexpr int blank_size = static_cast<int>(buffer_constant::buffer_blank);

    char* buffer_ = nullptr;
    int buffer_size_ = 0;
    int read_position_ = 0;
    int write_position_ = 0;
    CRITICAL_SECTION critical_section_{};
};

} // namespace dr
