# 0002. WiFi modem sleep stays off

Status: Accepted
Date: 2026-11-17

## Context

The ESP32 enables modem sleep by default. While asleep it can miss DTIM beacons and the group key rekey. UniFi APs then drop the client with reason 200 `BEACON_TIMEOUT` or 16 `GROUP_KEY_UPDATE_TIMEOUT`, which matched the drops seen in the field.

## Decision

Call `WiFi.setSleep(false)` in `wifiSetup()` and again after each connect, so a driver reset during reconnect cannot bring the default back.

## Consequences

- Higher idle current, because the radio stays on. The display is mains powered, so this is acceptable.
- The power supply must handle continuous radio current; brownout resets point at the supply, not the firmware (see `docs/wifi-troubleshooting.md`).
- Do not re-enable sleep to save power without testing on UniFi for several days.
