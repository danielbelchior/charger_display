#include <stdarg.h>
#include <time.h>

#include "logging.h"
#include "config.h"
#include "log_ring.h"

static LogRing<LOG_BUFFER_SIZE, LOG_LINE_LEN> ring;

// Any epoch before this means SNTP has not set the clock yet.
static constexpr time_t MIN_VALID_EPOCH = 1700000000;

static const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Error:
            return "ERROR";
    }
    return "INFO";
}

void formatClock(char* out, size_t len) {
    time_t now = time(nullptr);
    if (now >= MIN_VALID_EPOCH) {
        struct tm local;
        localtime_r(&now, &local);
        strftime(out, len, "%H:%M:%S", &local);
        return;
    }
    unsigned long seconds = millis() / 1000;
    snprintf(out, len, "+%02lu:%02lu:%02lu", seconds / 3600, (seconds / 60) % 60, seconds % 60);
}

void logMessage(LogLevel level, const char* format, ...) {
    char message[200];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    for (char* p = message; *p; ++p) {
        if (*p == '\n' || *p == '\r') {
            *p = ' ';
        }
    }

    char clock[16];
    formatClock(clock, sizeof(clock));

    char line[sizeof(message) + 32];
    snprintf(line, sizeof(line), "%s %s: %s", clock, levelName(level), message);
    Serial.println(line);

    ring.push(line);
}

size_t logCount() {
    return ring.size();
}

const char* logEntry(size_t index) {
    return ring.at(index);
}
