// Charger Display firmware: shows two Home Assistant charger states on an
// 8x8 WS2812B matrix. See docs/architecture.md for the module layout.

#include <ESPmDNS.h>
#include <esp_system.h>

#include "config.h"
#include "app_state.h"
#include "buzzer.h"
#include "display.h"
#include "hass.h"
#include "http_server.h"
#include "logging.h"
#include "ota.h"
#include "wifi_manager.h"

static uint32_t lastFrameAt = 0;
static bool networkServicesStarted = false;

static const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:
            return "power-on";
        case ESP_RST_SW:
            return "software";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt watchdog";
        case ESP_RST_TASK_WDT:
            return "task watchdog";
        case ESP_RST_WDT:
            return "watchdog";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_DEEPSLEEP:
            return "deep sleep";
        default:
            return "other";
    }
}

static void startNetworkServices() {
    configTzTime(TIMEZONE, NTP_SERVER);

    if (MDNS.begin(DEVICE_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        logInfo("mDNS: http://%s.local", DEVICE_HOSTNAME);
    } else {
        logError("mDNS: failed to start");
    }

    httpServerSetup();
    otaSetup();
    networkServicesStarted = true;
}

static void onWifiConnected() {
    app.wifiConnected = true;
    if (!networkServicesStarted) {
        startNetworkServices();
    }
    displayRender();
}

static void onWifiDisconnected() {
    app.wifiConnected = false;
    resetSensorStates();
    displayRender();
}

static void superviseHomeAssistant() {
    if (app.wifiConnected && hassDisconnectedForMs() >= HASS_RESTART_AFTER_MS) {
        restartDevice("Home Assistant unreachable for 10 minutes");
    }
}

void setup() {
    Serial.begin(115200);
    buzzerSetup();
    playSound(2, 20, 50);

    displaySetup();
    displayRender();

    logInfo("Boot: reset reason %s", resetReasonName(esp_reset_reason()));
    if (const char* reason = previousRestartReason()) {
        logInfo("Boot: previous restart requested because: %s", reason);
    }

    wifiSetup();
    playSound(3, 20, 50);
    playSound(1, 50, 400);
}

void loop() {
    if (otaLoop()) {
        return;
    }

    switch (wifiLoop()) {
        case WifiChange::Connected:
            onWifiConnected();
            break;
        case WifiChange::Disconnected:
            onWifiDisconnected();
            break;
        case WifiChange::None:
            break;
    }

    hassLoop();
    httpServerLoop();
    superviseHomeAssistant();

    const uint32_t now = millis();
    if (now - lastFrameAt >= DISPLAY_FRAME_MS) {
        lastFrameAt = now;
        displayTick();
    }
}
