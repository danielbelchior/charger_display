#pragma once

#include "config.h"

constexpr size_t SENSOR_COUNT = 3;
constexpr size_t SENSOR_STATE_LEN = 16;

// Sensor 1 and 2 are the two chargers shown on the matrix. Sensor 3 is the air
// compressor: on a state change it beeps (3 short on "on", one long otherwise)
// and scrolls a green "ON" or red "OFF" across the matrix.
extern const char* const SENSOR_ENTITY_IDS[SENSOR_COUNT];

struct AppState {
    char sensorState[SENSOR_COUNT][SENSOR_STATE_LEN];
    bool wifiConnected;
    bool hassConnected;
    uint8_t displayBrightness;
};

extern AppState app;

void setSensorState(size_t index, const char* state);

// Restart the chip, remembering why so the next boot can log it.
[[noreturn]] void restartDevice(const char* reason);
// Reason passed to restartDevice() before the current boot, or nullptr.
const char* previousRestartReason();
