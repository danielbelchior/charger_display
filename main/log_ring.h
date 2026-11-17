#pragma once

// Fixed-size ring of text lines; the oldest line is overwritten when full.
// Header-only and Arduino-free so it can be unit tested on the host.

#include <stddef.h>
#include <string.h>

template <size_t Capacity, size_t LineLen>
class LogRing {
    static_assert(Capacity > 0 && LineLen > 1, "LogRing needs room for at least one line");

public:
    // Stores a copy of `line`, truncated to LineLen - 1 characters.
    void push(const char* line) {
        strncpy(lines_[head_], line ? line : "", LineLen - 1);
        lines_[head_][LineLen - 1] = '\0';
        head_ = (head_ + 1) % Capacity;
        if (count_ < Capacity) {
            count_++;
        }
    }

    size_t size() const { return count_; }

    // 0 is the newest line. Returns nullptr when out of range.
    const char* at(size_t index) const {
        if (index >= count_) {
            return nullptr;
        }
        return lines_[(head_ + Capacity - 1 - index) % Capacity];
    }

private:
    char lines_[Capacity][LineLen] = {};
    size_t head_ = 0;
    size_t count_ = 0;
};
