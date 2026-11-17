# 0005. Decision logic lives in Arduino-free modules with host tests

Status: Accepted
Date: 2026-11-17

## Context

The WiFi reconnect bugs were timing and ordering problems (late events, the core's hidden retry, `millis()` wraparound). They were only visible on hardware after hours. The firmware could not be tested off-device because every module included `Arduino.h`.

## Decision

- Logic that decides something is written without Arduino or ESP-IDF headers: `wifi_policy.*` (reconnect state machine), `validation.*` (REST input parsing, HA on-state), `log_ring.h` (event log storage), `display_frame.*` (matrix patterns).
- Hardware modules are thin adapters that gather inputs, call the pure code, and perform its outputs.
- `tests/host/` holds unit tests for those modules with a small built-in harness (no framework download). `scripts/test.sh` builds and runs them; CI runs them before the firmware build.
- Time is passed in as a `uint32_t now` argument, never read inside pure code, so tests can step through time and across the 49-day `millis()` wraparound.

## Consequences

- Pure modules may include only the C/C++ standard library (`<stdint.h>`, `<string.h>`, ...). Adding `Arduino.h` to them breaks the host tests.
- New decision logic (for example HA reconnect timing) should follow the same split when it grows beyond a few lines.
- Host tests prove logic, not radio behaviour; WiFi changes still need a soak test on real UniFi hardware.
