#include <WiFi.h>

#include "wifi_manager.h"
#include "app_state.h"
#include "config.h"
#include "logging.h"

// Adapter between the ESP32 WiFi driver and WifiPolicy (wifi_policy.cpp),
// which holds all reconnect decisions. The core's auto-reconnect is off: it
// races our own begin() calls ("sta is connecting") and can wedge the driver.

static WifiPolicy policy({
    WIFI_CONNECT_TIMEOUT_MS,
    WIFI_RETRY_MIN_MS,
    WIFI_RETRY_MAX_MS,
    WIFI_RESTART_AFTER_MS,
});

static constexpr uint32_t SIGNAL_CHECK_INTERVAL_MS = 10000;
static uint32_t lastSignalCheckAt = 0;
static uint32_t lastWeakSignalLogAt = 0;
static bool weakSignalLogged = false;

// Written by the WiFi event task, consumed by the main loop.
static volatile bool gotIpPending = false;
static volatile bool disconnectPending = false;
static volatile uint8_t pendingReason = 0;

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    // Runs on the WiFi event task: record facts only.
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            gotIpPending = true;
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            pendingReason = info.wifi_sta_disconnected.reason;
            disconnectPending = true;
            break;
        default:
            break;
    }
}

static void logWeakSignal(uint32_t now) {
    if (now - lastSignalCheckAt < SIGNAL_CHECK_INTERVAL_MS) {
        return;
    }
    lastSignalCheckAt = now;
    const int32_t rssi = WiFi.RSSI();
    if (rssi >= WIFI_WEAK_RSSI_DBM) {
        return;
    }
    if (weakSignalLogged && now - lastWeakSignalLogAt < WIFI_WEAK_RSSI_LOG_INTERVAL_MS) {
        return;
    }
    weakSignalLogged = true;
    lastWeakSignalLogAt = now;
    logWarning("WiFi: weak signal %ld dBm from %s", (long)rssi, WiFi.BSSIDstr().c_str());
}

static void apply(const WifiStep& step) {
    const uint8_t reason = policy.lastReason();
    switch (step.note) {
        case WifiNote::Connected:
            weakSignalLogged = false;
            WiFi.setSleep(false);
            logInfo("WiFi: connected to %s ch %d, RSSI %ld dBm, IP %s", WiFi.BSSIDstr().c_str(), (int)WiFi.channel(),
                    (long)WiFi.RSSI(), WiFi.localIP().toString().c_str());
            break;
        case WifiNote::AttemptFailed:
            logWarning("WiFi: attempt failed, reason %u (%s)", reason, wifiReasonName(reason));
            break;
        case WifiNote::AttemptTimedOut:
            logWarning("WiFi: attempt timed out");
            break;
        case WifiNote::WaitingForCoreRetry:
            logInfo("WiFi: reason %u (%s), core is retrying once, waiting for it", reason, wifiReasonName(reason));
            break;
        case WifiNote::LinkLost:
            logWarning("WiFi: disconnected after %lus, reason %u (%s)", (unsigned long)(step.sessionMs / 1000), reason,
                       wifiReasonName(reason));
            break;
        case WifiNote::OfflineTooLong:
        case WifiNote::None:
            break;
    }
    if (step.retryInMs) {
        logInfo("WiFi: retry in %lus", (unsigned long)(step.retryInMs / 1000));
    }

    switch (step.action) {
        case WifiAction::Begin:
            logInfo("WiFi: connecting to '%s'", WIFI_SSID);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            break;
        case WifiAction::Disconnect:
            WiFi.disconnect();
            break;
        case WifiAction::Restart:
            restartDevice("WiFi offline for 5 minutes");
            break;
        case WifiAction::None:
            break;
    }
}

void wifiSetup() {
    WiFi.persistent(false);             // do not rewrite credentials to flash on every begin()
    WiFi.setHostname(DEVICE_HOSTNAME);  // must precede mode()/begin()
    WiFi.mode(WIFI_STA);
    // Modem sleep makes the ESP32 skip DTIM beacons and group-key rekeys;
    // UniFi APs then drop it (BEACON_TIMEOUT / GROUP_KEY_UPDATE_TIMEOUT).
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false);
    // Default fast scan joins the first AP heard on the lowest channel; with
    // several APs on one SSID that is often a distant one. Scan everything and
    // pick the strongest.
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.onEvent(onWifiEvent);

    apply(policy.start(millis()));
}

WifiChange wifiLoop() {
    WifiInputs in;
    in.gotIp = gotIpPending;
    gotIpPending = false;
    in.disconnected = disconnectPending;
    in.reason = pendingReason;
    disconnectPending = false;
    in.linkUp = WiFi.status() == WL_CONNECTED;

    const uint32_t now = millis();
    const WifiStep step = policy.tick(now, in);
    apply(step);
    if (policy.state() == WifiState::Connected && step.change == WifiChange::None) {
        logWeakSignal(now);
    }
    return step.change;
}

bool wifiIsConnected() {
    return policy.state() == WifiState::Connected;
}

uint32_t wifiConnectedForMs() {
    return policy.connectedForMs(millis());
}

uint32_t wifiDisconnectCount() {
    return policy.disconnectCount();
}

const char* wifiLastDisconnectReason() {
    return wifiReasonName(policy.lastReason());
}
