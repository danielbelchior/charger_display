#include "validation.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

bool parseBrightness(const char* text, uint8_t& out) {
    if (!text) {
        return false;
    }
    const size_t len = strlen(text);
    if (len == 0 || len > 3) {
        return false;
    }
    unsigned value = 0;
    for (size_t i = 0; i < len; i++) {
        if (!isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
        value = value * 10 + static_cast<unsigned>(text[i] - '0');
    }
    if (value > 255) {
        return false;
    }
    out = static_cast<uint8_t>(value);
    return true;
}

bool isValidSensorState(const char* text, size_t maxLen) {
    if (!text) {
        return false;
    }
    const size_t len = strlen(text);
    if (len == 0 || len >= maxLen) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (!isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

bool parseSensorId(const char* text, size_t count, size_t& out) {
    if (!text || strlen(text) != 1 || !isdigit(static_cast<unsigned char>(text[0]))) {
        return false;
    }
    const size_t id = static_cast<size_t>(text[0] - '0');
    if (id < 1 || id > count) {
        return false;
    }
    out = id;
    return true;
}

bool isOnState(const char* state) {
    return state && (strcasecmp(state, "on") == 0 || strcasecmp(state, "true") == 0);
}
