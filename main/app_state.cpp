#include <esp_attr.h>

#include "app_state.h"
#include "logging.h"

const char* const SENSOR_ENTITY_IDS[SENSOR_COUNT] = {
    SENSOR_1_ENTITY_ID,
    SENSOR_2_ENTITY_ID,
    SENSOR_3_ENTITY_ID,
};

AppState app = {
    {"unknown", "unknown", "unknown"},
    false,
    false,
    DEFAULT_BRIGHTNESS,
};

void setSensorState(size_t index, const char* state) {
    if (index >= SENSOR_COUNT) {
        return;
    }
    strlcpy(app.sensorState[index], state ? state : "unknown", SENSOR_STATE_LEN);
}

// RTC slow memory survives a software reset but not a power cycle, so the
// magic value tells us whether the note was written by restartDevice().
static constexpr uint32_t RESTART_NOTE_MAGIC = 0xC4A26E01;
static RTC_NOINIT_ATTR uint32_t restartNoteMagic;
static RTC_NOINIT_ATTR char restartNote[64];
static char previousNote[64] = "";
static bool previousNoteLoaded = false;

const char* previousRestartReason() {
    if (!previousNoteLoaded) {
        previousNoteLoaded = true;
        if (restartNoteMagic == RESTART_NOTE_MAGIC) {
            restartNote[sizeof(restartNote) - 1] = '\0';
            strlcpy(previousNote, restartNote, sizeof(previousNote));
        }
        restartNoteMagic = 0;
    }
    return previousNote[0] ? previousNote : nullptr;
}

void restartDevice(const char* reason) {
    logError("Restarting: %s", reason);
    strlcpy(restartNote, reason, sizeof(restartNote));
    restartNoteMagic = RESTART_NOTE_MAGIC;
    Serial.flush();
    delay(100);
    ESP.restart();
    while (true) {
        delay(1000);
    }
}
