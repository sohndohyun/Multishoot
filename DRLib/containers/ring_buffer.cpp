#include "containers/ring_buffer.hpp"

#include <cstring>

dr::ring_buffer::ring_buffer() {
    InitializeCriticalSection(&critical_section_);

    buffer_ = nullptr;
    buffer_size_ = 0;

    read_position_ = 0;
    write_position_ = 0;

    initialize(default_size);
}
dr::ring_buffer::ring_buffer(int buffer_size) {
    InitializeCriticalSection(&critical_section_);

    buffer_ = nullptr;
    buffer_size_ = 0;

    read_position_ = 0;
    write_position_ = 0;

    initialize(buffer_size);
}
dr::ring_buffer::~ring_buffer() {
    delete[] buffer_;
    buffer_ = nullptr;
    buffer_size_ = 0;

    read_position_ = 0;
    write_position_ = 0;
    DeleteCriticalSection(&critical_section_);
}

void dr::ring_buffer::initialize(int buffer_size) {
    delete[] buffer_;
    buffer_ = nullptr;
    buffer_size_ = 0;
    read_position_ = 0;
    write_position_ = 0;

    if (buffer_size <= blank_size)
        return;

    buffer_size_ = buffer_size;
    buffer_ = new char[buffer_size];
}

int dr::ring_buffer::buffer_size() const {
    if (nullptr != buffer_) {
        return buffer_size_ - blank_size;
    }

    return 0;
}

int dr::ring_buffer::used_size() const {
    if (read_position_ <= write_position_) {
        return write_position_ - read_position_;
    } else {
        return buffer_size_ - read_position_ + write_position_;
    }
}

int dr::ring_buffer::free_size() const {
    return buffer_size_ - (used_size() + blank_size);
}

int dr::ring_buffer::contiguous_read_size() const {
    if (read_position_ <= write_position_) {
        return write_position_ - read_position_;
    } else {
        return buffer_size_ - read_position_;
    }
}

int dr::ring_buffer::contiguous_write_size() const {
    if (write_position_ <= read_position_) {
        return (read_position_ - write_position_) - blank_size;
    } else {
        if (read_position_ < blank_size) {
            return (buffer_size_ - write_position_) - (blank_size - read_position_);
        } else {
            return buffer_size_ - write_position_;
        }
    }
}

int dr::ring_buffer::write(char* data, int size) {
    int write_capacity;

    if (free_size() < size)
        return 0;

    if (0 >= size)
        return 0;

    if (read_position_ <= write_position_) {
        write_capacity = buffer_size_ - write_position_;

        if (write_capacity >= size) {
            memcpy(buffer_ + write_position_, data, size);
            write_position_ += size;
        } else {
            memcpy(buffer_ + write_position_, data, write_capacity);
            memcpy(buffer_, data + write_capacity, size - write_capacity);
            write_position_ = size - write_capacity;
        }
    } else {
        memcpy(buffer_ + write_position_, data, size);
        write_position_ += size;
    }

    write_position_ = write_position_ == buffer_size_ ? 0 : write_position_;

    return size;
}
int dr::ring_buffer::read(char* destination, int size) {
    int read_capacity;

    if (used_size() < size)
        size = used_size();

    if (0 >= size)
        return 0;

    if (read_position_ <= write_position_) {
        memcpy(destination, buffer_ + read_position_, size);
        read_position_ += size;
    } else {
        read_capacity = buffer_size_ - read_position_;

        if (read_capacity >= size) {
            memcpy(destination, buffer_ + read_position_, size);
            read_position_ += size;
        } else {
            memcpy(destination, buffer_ + read_position_, read_capacity);
            memcpy(destination + read_capacity, buffer_, size - read_capacity);
            read_position_ = size - read_capacity;
        }
    }

    read_position_ = read_position_ == buffer_size_ ? 0 : read_position_;
    return size;
}
int dr::ring_buffer::peek(char* destination, int size) const {
    int read_capacity;
    if (used_size() < size)
        size = used_size();

    if (0 >= size)
        return 0;

    if (read_position_ <= write_position_) {
        memcpy(destination, buffer_ + read_position_, size);
    } else {
        read_capacity = buffer_size_ - read_position_;
        if (read_capacity >= size) {
            memcpy(destination, buffer_ + read_position_, size);
        } else {
            memcpy(destination, buffer_ + read_position_, read_capacity);
            memcpy(destination + read_capacity, buffer_, size - read_capacity);
        }
    }

    return size;
}

void dr::ring_buffer::remove_data(int size) {
    int read_capacity;

    if (used_size() < size)
        size = used_size();

    if (0 >= size)
        return;

    if (read_position_ < write_position_) {
        read_position_ += size;
    } else {
        read_capacity = buffer_size_ - read_position_;

        if (read_capacity >= size) {
            read_position_ += size;
        } else {
            read_position_ = size - read_capacity;
        }
    }

    read_position_ = read_position_ == buffer_size_ ? 0 : read_position_;
}

void dr::ring_buffer::clear() {
    read_position_ = 0;
    write_position_ = 0;
}

char* dr::ring_buffer::buffer() {
    return buffer_;
}

char* dr::ring_buffer::read_buffer() {
    return buffer_ + read_position_;
}

char* dr::ring_buffer::write_buffer() {
    return buffer_ + write_position_;
}

void dr::ring_buffer::lock() {
    EnterCriticalSection(&critical_section_);
}

void dr::ring_buffer::unlock() {
    LeaveCriticalSection(&critical_section_);
}
