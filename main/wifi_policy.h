#pragma once

// Pure WiFi reconnect logic, free of Arduino/ESP-IDF calls so it can be unit
// tested on the host (tests/host). wifi_manager.cpp feeds it driver events and
// carries out the actions it returns.

#include <stdint.h>

enum class WifiChange : uint8_t { None, Connected, Disconnected };

// What the adapter must do with the driver after a tick.
enum class WifiAction : uint8_t { None, Begin, Disconnect, Restart };

// Why the tick did what it did, for logging.
enum class WifiNote : uint8_t {
    None,
    Connected,
    AttemptFailed,
    AttemptTimedOut,
    WaitingForCoreRetry,
    LinkLost,
    OfflineTooLong,
};

enum class WifiState : uint8_t { Waiting, Connecting, Connected };

struct WifiPolicyConfig {
    uint32_t connectTimeoutMs;
    uint32_t retryMinMs;
    uint32_t retryMaxMs;
    uint32_t restartAfterMs;
};

// Driver facts gathered since the previous tick.
struct WifiInputs {
    bool gotIp = false;         // ARDUINO_EVENT_WIFI_STA_GOT_IP seen
    bool disconnected = false;  // ARDUINO_EVENT_WIFI_STA_DISCONNECTED seen
    uint8_t reason = 0;         // reason of that disconnect event
    bool linkUp = false;        // WiFi.status() == WL_CONNECTED
};

struct WifiStep {
    WifiAction action = WifiAction::None;
    WifiChange change = WifiChange::None;
    WifiNote note = WifiNote::None;
    uint32_t retryInMs = 0;  // set when a retry was scheduled
    uint32_t sessionMs = 0;  // set with LinkLost: how long the link was up
};

constexpr uint8_t WIFI_REASON_ASSOC_LEAVE = 8;

const char* wifiReasonName(uint8_t reason);

class WifiPolicy {
public:
    explicit WifiPolicy(const WifiPolicyConfig& config);

    // First connection attempt; always returns Begin.
    WifiStep start(uint32_t now);
    WifiStep tick(uint32_t now, const WifiInputs& in);

    WifiState state() const { return state_; }
    uint32_t connectedForMs(uint32_t now) const;
    uint32_t disconnectCount() const { return disconnectCount_; }
    uint8_t lastReason() const { return lastReason_; }
    uint32_t retryDelayMs() const { return retryDelay_; }

private:
    void beginAttempt(uint32_t now);
    void enterConnected(uint32_t now);
    uint32_t scheduleRetry(uint32_t now);
    bool consumeCoreRetry(uint8_t reason, uint32_t now);

    WifiPolicyConfig config_;
    WifiState state_ = WifiState::Waiting;
    uint32_t attemptStartedAt_ = 0;
    uint32_t nextAttemptAt_ = 0;
    uint32_t retryDelay_ = 0;
    uint32_t connectedAt_ = 0;
    uint32_t offlineSince_ = 0;
    uint32_t disconnectCount_ = 0;
    uint8_t lastReason_ = 0;
    bool gotIp_ = false;
    // The ESP32 core retries the first disconnect of each boot by itself
    // (even with auto-reconnect off); we wait for it instead of racing it.
    bool coreRetryPending_ = true;
};
