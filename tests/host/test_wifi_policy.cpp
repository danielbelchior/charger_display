#include "../../main/wifi_policy.h"
#include "test.h"

// Same values as config.h, so the tests describe the shipped behaviour.
static const WifiPolicyConfig kConfig = {20000, 1000, 30000, 300000};

static WifiInputs idle() {
    return WifiInputs{};
}

static WifiInputs linkUpWithIp() {
    WifiInputs in;
    in.gotIp = true;
    in.linkUp = true;
    return in;
}

static WifiInputs dropped(uint8_t reason) {
    WifiInputs in;
    in.disconnected = true;
    in.reason = reason;
    return in;
}

static WifiInputs droppedWithLinkUp(uint8_t reason) {
    WifiInputs in = dropped(reason);
    in.linkUp = true;
    return in;
}

constexpr uint8_t BEACON_TIMEOUT = 200;
constexpr uint8_t NO_AP_FOUND = 201;
constexpr uint8_t GROUP_KEY_UPDATE_TIMEOUT = 16;

// Fails the first attempt twice so the one-time core retry is used up, leaving
// the policy Waiting with the next attempt due at `now` + returned delay.
static uint32_t burnCoreRetry(WifiPolicy& policy, uint32_t now) {
    policy.tick(now, dropped(NO_AP_FOUND));
    return policy.tick(now, dropped(NO_AP_FOUND)).retryInMs;
}

static void connect(WifiPolicy& policy, uint32_t now) {
    const WifiStep step = policy.tick(now, linkUpWithIp());
    CHECK(step.change == WifiChange::Connected);
    CHECK(policy.state() == WifiState::Connected);
}

TEST_CASE(start_begins_first_attempt) {
    WifiPolicy policy(kConfig);
    const WifiStep step = policy.start(0);
    CHECK(step.action == WifiAction::Begin);
    CHECK(policy.state() == WifiState::Connecting);
    CHECK_EQ(policy.connectedForMs(0), 0u);
}

TEST_CASE(got_ip_with_link_up_connects) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    const WifiStep step = policy.tick(1500, linkUpWithIp());
    CHECK(step.change == WifiChange::Connected);
    CHECK(step.note == WifiNote::Connected);
    CHECK(step.action == WifiAction::None);
    CHECK_EQ(policy.connectedForMs(4500), 3000u);
}

TEST_CASE(got_ip_is_remembered_until_link_reports_up) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    WifiInputs ipOnly;
    ipOnly.gotIp = true;
    CHECK(policy.tick(100, ipOnly).change == WifiChange::None);
    CHECK(policy.state() == WifiState::Connecting);
    WifiInputs linkOnly;
    linkOnly.linkUp = true;
    CHECK(policy.tick(200, linkOnly).change == WifiChange::Connected);
}

TEST_CASE(link_up_without_got_ip_does_not_connect_while_connecting) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    WifiInputs linkOnly;
    linkOnly.linkUp = true;
    CHECK(policy.tick(100, linkOnly).change == WifiChange::None);
    CHECK(policy.state() == WifiState::Connecting);
}

TEST_CASE(first_failure_of_boot_waits_for_core_retry) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    const WifiStep step = policy.tick(100, dropped(NO_AP_FOUND));
    CHECK(step.note == WifiNote::WaitingForCoreRetry);
    CHECK(step.action == WifiAction::None);
    CHECK_EQ(step.retryInMs, 0u);
    CHECK(policy.state() == WifiState::Connecting);
    CHECK_EQ(policy.lastReason(), NO_AP_FOUND);
}

TEST_CASE(second_failure_schedules_retry_with_backoff) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    policy.tick(100, dropped(NO_AP_FOUND));
    const WifiStep failed = policy.tick(200, dropped(NO_AP_FOUND));
    CHECK(failed.note == WifiNote::AttemptFailed);
    CHECK_EQ(failed.retryInMs, 1000u);
    CHECK(policy.state() == WifiState::Waiting);

    CHECK(policy.tick(1199, idle()).action == WifiAction::None);
    CHECK(policy.tick(1200, idle()).action == WifiAction::Begin);
    CHECK(policy.state() == WifiState::Connecting);
}

TEST_CASE(backoff_doubles_and_caps_at_retry_max) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    uint32_t now = 0;
    uint32_t delay = burnCoreRetry(policy, now);
    const uint32_t expected[] = {2000, 4000, 8000, 16000, 30000, 30000};
    CHECK_EQ(delay, 1000u);
    for (uint32_t want : expected) {
        now += delay;
        CHECK(policy.tick(now, idle()).action == WifiAction::Begin);
        delay = policy.tick(now, dropped(NO_AP_FOUND)).retryInMs;
        CHECK_EQ(delay, want);
    }
}

TEST_CASE(connecting_resets_backoff) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    uint32_t now = 0;
    uint32_t delay = burnCoreRetry(policy, now);
    now += delay;
    policy.tick(now, idle());
    delay = policy.tick(now, dropped(NO_AP_FOUND)).retryInMs;
    CHECK_EQ(delay, 2000u);
    now += delay;
    policy.tick(now, idle());
    connect(policy, now + 10);
    CHECK_EQ(policy.retryDelayMs(), 1000u);
}

TEST_CASE(attempt_times_out_and_resets_driver) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    CHECK(policy.tick(19999, idle()).note == WifiNote::None);
    const WifiStep step = policy.tick(20000, idle());
    CHECK(step.note == WifiNote::AttemptTimedOut);
    CHECK(step.action == WifiAction::Disconnect);
    CHECK_EQ(step.retryInMs, 1000u);
    CHECK(policy.state() == WifiState::Waiting);
}

TEST_CASE(late_assoc_leave_does_not_abort_attempt) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    const WifiStep step = policy.tick(100, dropped(kReasonAssocLeave));
    CHECK(step.note == WifiNote::None);
    CHECK(policy.state() == WifiState::Connecting);
    // ...and does not use up the core retry.
    CHECK(policy.tick(200, dropped(NO_AP_FOUND)).note == WifiNote::WaitingForCoreRetry);
}

TEST_CASE(core_retry_success_connects) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    policy.tick(100, dropped(NO_AP_FOUND));
    connect(policy, 3000);
}

TEST_CASE(got_ip_while_waiting_connects) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    burnCoreRetry(policy, 0);
    CHECK(policy.state() == WifiState::Waiting);
    connect(policy, 500);
}

TEST_CASE(first_drop_after_connect_uses_core_retry) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    connect(policy, 1000);
    const WifiStep step = policy.tick(61000, dropped(BEACON_TIMEOUT));
    CHECK(step.change == WifiChange::Disconnected);
    CHECK(step.note == WifiNote::LinkLost);
    CHECK(step.action == WifiAction::None);
    CHECK_EQ(step.sessionMs, 60000u);
    CHECK_EQ(policy.disconnectCount(), 1u);
    CHECK_EQ(policy.lastReason(), BEACON_TIMEOUT);
    CHECK(policy.state() == WifiState::Connecting);
    connect(policy, 62000);
}

TEST_CASE(ap_drop_reconnects_on_next_tick) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    burnCoreRetry(policy, 0);
    policy.tick(1000, idle());
    connect(policy, 1500);

    const WifiStep lost = policy.tick(90000, dropped(GROUP_KEY_UPDATE_TIMEOUT));
    CHECK(lost.change == WifiChange::Disconnected);
    CHECK(lost.action == WifiAction::None);
    CHECK(policy.state() == WifiState::Waiting);
    CHECK(policy.tick(90000, idle()).action == WifiAction::Begin);
}

TEST_CASE(drop_event_counts_even_if_link_still_reports_up) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    burnCoreRetry(policy, 0);
    policy.tick(1000, idle());
    connect(policy, 1500);
    CHECK(policy.tick(5000, droppedWithLinkUp(BEACON_TIMEOUT)).change == WifiChange::Disconnected);
}

TEST_CASE(assoc_leave_while_connected_is_a_real_drop) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    connect(policy, 1000);
    const WifiStep lost = policy.tick(5000, dropped(kReasonAssocLeave));
    CHECK(lost.change == WifiChange::Disconnected);
    // ASSOC_LEAVE never waits for the core retry.
    CHECK(policy.state() == WifiState::Waiting);
    CHECK(policy.tick(5000, idle()).action == WifiAction::Begin);
}

TEST_CASE(silent_link_loss_resets_driver_then_retries) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    connect(policy, 1000);
    const WifiStep lost = policy.tick(5000, idle());
    CHECK(lost.change == WifiChange::Disconnected);
    CHECK(lost.action == WifiAction::Disconnect);
    CHECK_EQ(policy.lastReason(), 0);
    CHECK(policy.state() == WifiState::Waiting);
    CHECK(policy.tick(5999, idle()).action == WifiAction::None);
    CHECK(policy.tick(6000, idle()).action == WifiAction::Begin);
}

TEST_CASE(got_ip_seen_while_connected_is_not_reused_later) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    connect(policy, 1000);
    WifiInputs renew;
    renew.gotIp = true;
    renew.linkUp = true;
    policy.tick(2000, renew);   // DHCP renewal while connected
    policy.tick(3000, idle());  // silent link loss -> Waiting until 4000
    WifiInputs linkOnly;
    linkOnly.linkUp = true;
    CHECK(policy.tick(3500, linkOnly).change == WifiChange::None);
}

TEST_CASE(restarts_after_five_minutes_offline) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    uint32_t restartAt = 0;
    for (uint32_t now = 100; now <= 400000; now += 100) {
        if (policy.tick(now, idle()).action == WifiAction::Restart) {
            restartAt = now;
            break;
        }
    }
    CHECK_EQ(restartAt, 300000u);
}

TEST_CASE(offline_timer_starts_at_link_loss) {
    WifiPolicy policy(kConfig);
    policy.start(0);
    connect(policy, 1000);
    policy.tick(1000000, dropped(BEACON_TIMEOUT));
    uint32_t restartAt = 0;
    for (uint32_t now = 1000100; now <= 1400000; now += 100) {
        if (policy.tick(now, idle()).action == WifiAction::Restart) {
            restartAt = now;
            break;
        }
    }
    CHECK_EQ(restartAt, 1300000u);
}

TEST_CASE(timers_survive_millis_wraparound) {
    WifiPolicy policy(kConfig);
    const uint32_t t0 = 0xFFFFF000u;  // 4096 ms before wrap
    policy.start(t0);
    CHECK(policy.tick(t0 + 19999, idle()).note == WifiNote::None);
    const WifiStep timedOut = policy.tick(t0 + 20000, idle());
    CHECK(timedOut.note == WifiNote::AttemptTimedOut);
    CHECK(policy.tick(t0 + 20999, idle()).action == WifiAction::None);
    CHECK(policy.tick(t0 + 21000, idle()).action == WifiAction::Begin);
    connect(policy, t0 + 22000);
    CHECK_EQ(policy.connectedForMs(t0 + 32000), 10000u);
}

TEST_CASE(retry_scheduled_across_wraparound_waits) {
    WifiPolicy policy(kConfig);
    const uint32_t t0 = 0xFFFFFF00u;  // 256 ms before wrap; retry lands after it
    policy.start(t0);
    CHECK_EQ(burnCoreRetry(policy, t0), 1000u);
    CHECK(policy.tick(t0 + 10, idle()).action == WifiAction::None);
    CHECK(policy.tick(t0 + 999, idle()).action == WifiAction::None);
    CHECK(policy.tick(t0 + 1000, idle()).action == WifiAction::Begin);
}

TEST_CASE(reason_names) {
    CHECK_STR(wifiReasonName(0), "none");
    CHECK_STR(wifiReasonName(kReasonAssocLeave), "ASSOC_LEAVE");
    CHECK_STR(wifiReasonName(BEACON_TIMEOUT), "BEACON_TIMEOUT");
    CHECK_STR(wifiReasonName(GROUP_KEY_UPDATE_TIMEOUT), "GROUP_KEY_UPDATE_TIMEOUT");
    CHECK_STR(wifiReasonName(99), "OTHER");
}
