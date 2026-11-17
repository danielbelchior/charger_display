#include "../../main/log_ring.h"
#include "test.h"

TEST_CASE(log_ring_starts_empty) {
    LogRing<3, 8> ring;
    CHECK_EQ(ring.size(), 0u);
    CHECK(ring.at(0) == nullptr);
}

TEST_CASE(log_ring_index_zero_is_newest) {
    LogRing<3, 8> ring;
    ring.push("a");
    ring.push("b");
    CHECK_EQ(ring.size(), 2u);
    CHECK_STR(ring.at(0), "b");
    CHECK_STR(ring.at(1), "a");
    CHECK(ring.at(2) == nullptr);
}

TEST_CASE(log_ring_overwrites_oldest_when_full) {
    LogRing<3, 8> ring;
    const char* lines[] = {"1", "2", "3", "4", "5"};
    for (const char* line : lines) {
        ring.push(line);
    }
    CHECK_EQ(ring.size(), 3u);
    CHECK_STR(ring.at(0), "5");
    CHECK_STR(ring.at(1), "4");
    CHECK_STR(ring.at(2), "3");
    CHECK(ring.at(3) == nullptr);
}

TEST_CASE(log_ring_truncates_long_lines) {
    LogRing<2, 5> ring;
    ring.push("abcdefgh");
    CHECK_STR(ring.at(0), "abcd");
}

TEST_CASE(log_ring_treats_null_as_empty_line) {
    LogRing<2, 5> ring;
    ring.push(nullptr);
    CHECK_EQ(ring.size(), 1u);
    CHECK_STR(ring.at(0), "");
}

TEST_CASE(log_ring_wraps_many_times) {
    LogRing<4, 8> ring;
    char line[8];
    for (int i = 0; i < 1003; i++) {
        snprintf(line, sizeof line, "%d", i);
        ring.push(line);
    }
    CHECK_EQ(ring.size(), 4u);
    CHECK_STR(ring.at(0), "1002");
    CHECK_STR(ring.at(3), "999");
}
