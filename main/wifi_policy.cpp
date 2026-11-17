#include "wifi_policy.h"

// clang-format off
const char* wifiReasonName(uint8_t reason) {
    switch (reason) {
        case 0: return "none";
        case 1: return "UNSPECIFIED";
        case 2: return "AUTH_EXPIRE";
        case 3: return "AUTH_LEAVE";
        case 4: return "ASSOC_EXPIRE";
        case 5: return "ASSOC_TOOMANY";
        case 6: return "NOT_AUTHED";
        case 7: return "NOT_ASSOCED";
        case 8: return "ASSOC_LEAVE";
        case 15: return "4WAY_HANDSHAKE_TIMEOUT";
        case 16: return "GROUP_KEY_UPDATE_TIMEOUT";
        case 23: return "802_1X_AUTH_FAILED";
        case 200: return "BEACON_TIMEOUT";
        case 201: return "NO_AP_FOUND";
        case 202: return "AUTH_FAIL";
        case 203: return "ASSOC_FAIL";
        case 204: return "HANDSHAKE_TIMEOUT";
        case 205: return "CONNECTION_FAIL";
        case 210: return "NO_AP_FOUND_W_COMPATIBLE_SECURITY";
        case 211: return "NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
        case 212: return "NO_AP_FOUND_IN_RSSI_THRESHOLD";
        default: return "OTHER";
    }
}
// clang-format on

WifiPolicy::WifiPolicy(const WifiPolicyConfig& config) : config_(config), retryDelay_(config.retryMinMs) {}

WifiStep WifiPolicy::start(uint32_t now) {
    offlineSince_ = now;
    beginAttempt(now);
    WifiStep step;
    step.action = WifiAction::Begin;
    return step;
}

uint32_t WifiPolicy::connectedForMs(uint32_t now) const {
    return state_ == WifiState::Connected ? now - connectedAt_ : 0;
}

void WifiPolicy::beginAttempt(uint32_t now) {
    gotIp_ = false;
    attemptStartedAt_ = now;
    state_ = WifiState::Connecting;
}

void WifiPolicy::enterConnected(uint32_t now) {
    gotIp_ = false;
    state_ = WifiState::Connected;
    connectedAt_ = now;
    retryDelay_ = config_.retryMinMs;
}

uint32_t WifiPolicy::scheduleRetry(uint32_t now) {
    const uint32_t delay = retryDelay_;
    nextAttemptAt_ = now + delay;
    retryDelay_ = retryDelay_ * 2 > config_.retryMaxMs ? config_.retryMaxMs : retryDelay_ * 2;
    state_ = WifiState::Waiting;
    return delay;
}

bool WifiPolicy::consumeCoreRetry(uint8_t reason, uint32_t now) {
    if (!coreRetryPending_ || reason == kReasonAssocLeave) {
        return false;
    }
    coreRetryPending_ = false;
    attemptStartedAt_ = now;
    state_ = WifiState::Connecting;
    return true;
}

WifiStep WifiPolicy::tick(uint32_t now, const WifiInputs& in) {
    WifiStep step;

    // ASSOC_LEAVE outside Connected is our own (or the core's) disconnect()
    // landing late; it must not abort an attempt that started after it.
    const bool disconnected = in.disconnected && !(in.reason == kReasonAssocLeave && state_ != WifiState::Connected);
    if (state_ != WifiState::Connected) {
        gotIp_ = gotIp_ || in.gotIp;
    }

    // An IP can come from the core's own retry, whatever our state.
    if (state_ != WifiState::Connected && gotIp_ && in.linkUp) {
        enterConnected(now);
        step.change = WifiChange::Connected;
        step.note = WifiNote::Connected;
        return step;
    }

    switch (state_) {
        case WifiState::Waiting:
            if ((int32_t)(now - nextAttemptAt_) >= 0) {
                if (in.linkUp) {
                    enterConnected(now);
                    step.change = WifiChange::Connected;
                    step.note = WifiNote::Connected;
                    return step;
                }
                beginAttempt(now);
                step.action = WifiAction::Begin;
            }
            break;

        case WifiState::Connecting:
            if (disconnected) {
                lastReason_ = in.reason;
                if (consumeCoreRetry(in.reason, now)) {
                    step.note = WifiNote::WaitingForCoreRetry;
                } else {
                    step.note = WifiNote::AttemptFailed;
                    step.retryInMs = scheduleRetry(now);
                }
            } else if (now - attemptStartedAt_ >= config_.connectTimeoutMs) {
                step.note = WifiNote::AttemptTimedOut;
                step.action = WifiAction::Disconnect;
                step.retryInMs = scheduleRetry(now);
            }
            break;

        case WifiState::Connected:
            if (disconnected || !in.linkUp) {
                lastReason_ = disconnected ? in.reason : 0;
                disconnectCount_++;
                offlineSince_ = now;
                retryDelay_ = config_.retryMinMs;
                step.change = WifiChange::Disconnected;
                step.note = WifiNote::LinkLost;
                step.sessionMs = now - connectedAt_;
                if (disconnected && consumeCoreRetry(in.reason, now)) {
                    // Core is already reconnecting; we are in Connecting now.
                } else if (disconnected) {
                    // AP-initiated drops (kick, roam, rekey) usually recover at once.
                    nextAttemptAt_ = now;
                    state_ = WifiState::Waiting;
                } else {
                    // Link went bad without an event: reset the driver and let its
                    // disconnect event land before the next attempt.
                    step.action = WifiAction::Disconnect;
                    nextAttemptAt_ = now + config_.retryMinMs;
                    state_ = WifiState::Waiting;
                }
            }
            return step;
    }

    if (now - offlineSince_ >= config_.restartAfterMs) {
        step.action = WifiAction::Restart;
        step.note = WifiNote::OfflineTooLong;
    }
    return step;
}
