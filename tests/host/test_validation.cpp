#include "../../main/validation.h"
#include "test.h"

TEST_CASE(brightness_accepts_0_to_255) {
    uint8_t out = 7;
    CHECK(parseBrightness("0", out));
    CHECK_EQ(out, 0);
    CHECK(parseBrightness("255", out));
    CHECK_EQ(out, 255);
    CHECK(parseBrightness("042", out));
    CHECK_EQ(out, 42);
}

TEST_CASE(brightness_rejects_bad_input_and_keeps_output) {
    uint8_t out = 7;
    CHECK(!parseBrightness("256", out));
    CHECK(!parseBrightness("999", out));
    CHECK(!parseBrightness("1000", out));
    CHECK(!parseBrightness("", out));
    CHECK(!parseBrightness("-1", out));
    CHECK(!parseBrightness("12a", out));
    CHECK(!parseBrightness(" 12", out));
    CHECK(!parseBrightness(nullptr, out));
    CHECK_EQ(out, 7);
}

TEST_CASE(sensor_state_accepts_token_characters) {
    CHECK(isValidSensorState("on", 16));
    CHECK(isValidSensorState("not_home", 16));
    CHECK(isValidSensorState("Charging-2", 16));
}

TEST_CASE(sensor_state_rejects_empty_long_and_unsafe) {
    CHECK(!isValidSensorState("", 16));
    CHECK(!isValidSensorState(nullptr, 16));
    CHECK(!isValidSensorState("has space", 16));
    CHECK(!isValidSensorState("<script>", 16));
    CHECK(!isValidSensorState("a/b", 16));
    // maxLen counts the NUL terminator.
    CHECK(isValidSensorState("abc", 4));
    CHECK(!isValidSensorState("abcd", 4));
}

TEST_CASE(sensor_id_accepts_1_to_count) {
    size_t id = 0;
    CHECK(parseSensorId("1", 3, id));
    CHECK_EQ(id, 1u);
    CHECK(parseSensorId("3", 3, id));
    CHECK_EQ(id, 3u);
}

TEST_CASE(sensor_id_rejects_out_of_range_and_garbage) {
    size_t id = 9;
    CHECK(!parseSensorId("0", 3, id));
    CHECK(!parseSensorId("4", 3, id));
    CHECK(!parseSensorId("10", 3, id));
    CHECK(!parseSensorId("01", 3, id));
    CHECK(!parseSensorId("a", 3, id));
    CHECK(!parseSensorId("", 3, id));
    CHECK(!parseSensorId(nullptr, 3, id));
    CHECK_EQ(id, 9u);
}

TEST_CASE(on_state_matches_on_and_true_case_insensitive) {
    CHECK(isOnState("on"));
    CHECK(isOnState("ON"));
    CHECK(isOnState("true"));
    CHECK(isOnState("True"));
    CHECK(!isOnState("off"));
    CHECK(!isOnState("unavailable"));
    CHECK(!isOnState("onn"));
    CHECK(!isOnState(""));
    CHECK(!isOnState(nullptr));
}
