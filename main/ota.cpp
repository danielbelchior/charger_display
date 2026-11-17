#include "ota.h"
#include "config.h"

#if ENABLE_OTA

#include <ArduinoOTA.h>

#include "buzzer.h"
#include "display.h"
#include "logging.h"

static bool started = false;
static bool inProgress = false;
static unsigned lastReportedDecile = 0;

void otaSetup() {
    if (started) {
        return;
    }
    started = true;

    ArduinoOTA.setHostname(DEVICE_HOSTNAME);
#ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
#endif
    ArduinoOTA.onStart([]() {
        inProgress = true;
        lastReportedDecile = 0;
        displaySetOtaInProgress(true);
        logInfo("OTA: start (%s)", ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem");
        playSound(3, 100, 100);
    });
    ArduinoOTA.onEnd([]() {
        inProgress = false;
        logInfo("OTA: done");
        playSound(5, 100, 100);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        const unsigned decile = total ? (progress * 10U) / total : 0;
        if (decile > lastReportedDecile) {
            lastReportedDecile = decile;
            logInfo("OTA: %u%%", decile * 10);
            playSound(1, 100, 100);
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        inProgress = false;
        displaySetOtaInProgress(false);
        logError("OTA: error %u", (unsigned)error);
        playSound(1, 300, 300);
    });
    ArduinoOTA.begin();
    logInfo("OTA: ready");
}

bool otaLoop() {
    if (started) {
        ArduinoOTA.handle();
    }
    return inProgress;
}

#else

void otaSetup() {}
bool otaLoop() {
    return false;
}

#endif
