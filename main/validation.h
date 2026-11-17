#pragma once

// Input parsing and Home Assistant state helpers. Arduino-free so they can be
// unit tested on the host.

#include <stddef.h>
#include <stdint.h>

// Accepts 1-3 decimal digits with a value of 0..255.
bool parseBrightness(const char* text, uint8_t& out);

// Accepts 1..maxLen-1 characters from [A-Za-z0-9_-] (maxLen includes the NUL).
bool isValidSensorState(const char* text, size_t maxLen);

// Accepts "1".."count" and returns the 1-based id.
bool parseSensorId(const char* text, size_t count, size_t& out);

// HA binary sensors report "on"; some integrations report "true". Case-insensitive.
bool isOnState(const char* state);
