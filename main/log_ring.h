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
        const char* text = line ? line : "";
        const size_t len = strnlen(text, LineLen - 1);
        memcpy(lines_[head_], text, len);
        lines_[head_][len] = '\0';
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
