#include <ArduinoOTA.h>
#include "logging.h"
#include "util.h"
#include "declarations.h"
#include "display.h"

void setupOTA(const char* hostname) {
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        ota_in_progress = true;
        render_matrix();
        logInfo("Start updating " + type);
        playSound(3, 100, 100);
    });
    ArduinoOTA.onEnd([]() {
        ota_in_progress = false;
        playSound(5, 100, 100);
        logInfo("End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        logInfo("Progress: " + String((progress / (total / 100))) + "%");
        if ( (progress / (total / 100)) % 10 ==0 ){
            playSound(1, 100, 100);
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        ota_in_progress = false;
        playSound(1, 300, 300);
        logError("Error[" + String(error) + "]: ");
        if (error == OTA_AUTH_ERROR) logError("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) logError("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) logError("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) logError("Receive Failed");
        else if (error == OTA_END_ERROR) logError("End Failed");
    });
    ArduinoOTA.begin();
}

void handleOTA() {
    ArduinoOTA.handle();
}
