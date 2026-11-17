#pragma once

#include <Arduino.h>

// Logs go to Serial and to an in-memory ring buffer exposed by the web UI.
// Only call from the main loop task: the ring buffer is not thread-safe.

enum class LogLevel : uint8_t { Debug, Info, Warning, Error };

void logMessage(LogLevel level, const char* format, ...) __attribute__((format(printf, 2, 3)));

#define logDebug(...) logMessage(LogLevel::Debug, __VA_ARGS__)
#define logInfo(...) logMessage(LogLevel::Info, __VA_ARGS__)
#define logWarning(...) logMessage(LogLevel::Warning, __VA_ARGS__)
#define logError(...) logMessage(LogLevel::Error, __VA_ARGS__)

size_t logCount();
// 0 is the newest entry. Returns nullptr when out of range.
const char* logEntry(size_t index);

// Wall-clock "HH:MM:SS" once NTP has synced, uptime "+HH:MM:SS" before that.
void formatClock(char* out, size_t len);
