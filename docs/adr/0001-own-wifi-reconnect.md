# 0001. The firmware owns WiFi reconnects

Status: Accepted
Date: 2026-11-17

## Context

The display dropped off UniFi networks at random. Logs showed `sta is connecting, return error` and occasional wedged drivers. The ESP32 core's auto-reconnect and the sketch's own `WiFi.begin()` calls ran at the same time and fought over the driver. The old watchdog also rebooted after 30 s of any outage, so AP firmware updates caused reboot loops.

## Decision

- `WiFi.setAutoReconnect(false)`. Only `WifiPolicy` (`main/wifi_policy.*`) decides when to call `WiFi.begin()`, `WiFi.disconnect()`, or restart.
- One attempt at a time, 20 s connect timeout, backoff 1 s to 30 s.
- Wait for the core's one built-in retry of the first disconnect of each boot instead of starting a parallel attempt.
- Ignore `ASSOC_LEAVE` (reason 8) unless Connected; it is the echo of our own `disconnect()`.
- Reboot only after 5 minutes offline.

## Consequences

- Any new code must not call `WiFi.begin()`, `WiFi.reconnect()`, or `setAutoReconnect(true)`; add behaviour to `WifiPolicy` instead and cover it in `tests/host/test_wifi_policy.cpp`.
- The WiFi event callback only records facts (flags); all decisions happen on the main loop.
- If a future core version stops doing its built-in retry, the policy still works: the pending core retry is consumed by the next failure, which then costs one extra connect timeout at most once per boot.
