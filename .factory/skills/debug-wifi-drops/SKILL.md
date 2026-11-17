---
name: debug-wifi-drops
description: Diagnose WiFi disconnects of the charger_display ESP32, especially on Ubiquiti/UniFi networks. Use when the user reports the display dropping WiFi, showing the no-WiFi or no-HA pattern, rebooting, or shares its logs or /status output.
---

# Debug WiFi drops

Background: [docs/wifi-troubleshooting.md](../../../docs/wifi-troubleshooting.md), reconnect design in [ADR 0001](../../../docs/adr/0001-own-wifi-reconnect.md).

## 1. Gather evidence

Ask for (or fetch, if the device is reachable and the user agrees):

- `http://<device>/status`: `wifi_rssi`, `wifi_bssid`, `wifi_channel`, `wifi_disconnects`, `wifi_last_disconnect_reason`, `uptime`, `logBuffer`.
- Serial log from boot if available (115200 baud). The first lines show `Boot: reset reason ...` and `Boot: previous restart requested because: ...`.
- AP model and the SSID settings listed in the troubleshooting doc.

## 2. Classify

| Evidence | Likely cause | Next step |
| --- | --- | --- |
| Reason 200 `BEACON_TIMEOUT`, RSSI below -75 dBm | Weak link or wrong AP | Check BSSID; lock to the near AP or move the device |
| Reason 16 `GROUP_KEY_UPDATE_TIMEOUT` | Missed rekey | Confirm `setSleep(false)` is still in place; raise the UniFi group rekey interval |
| Reason 2 / 4 at regular intervals | AP removes idle clients, or AP restarts | Check UniFi events at the same timestamps |
| Reason 15 / 204 | Password or WPA3/PMF mismatch | Use WPA2 or transitional mode |
| Reason 201 `NO_AP_FOUND` | SSID not on 2.4 GHz or out of range | Check band settings |
| BSSID changes between connects | Roaming between APs | Lock to AP, lower far AP's TX power |
| Reset reason `brownout` | Power supply | Better 5 V supply, 470 µF capacitor |
| WiFi stays up but HA pattern shows | Dead TCP session or HA down | Check `HASS:` log lines for idle timeout / auth errors |
| `restart requested because: WiFi offline for 5 minutes` | Long outage | Look at what the AP did in that window |

## 3. Changing the firmware

Only if the evidence points at firmware behaviour:

1. Reproduce the event sequence as a test in `tests/host/test_wifi_policy.cpp` (events and `now` timestamps from the log).
2. Fix `main/wifi_policy.cpp`; keep driver calls in `main/wifi_manager.cpp`.
3. Run `scripts/test.sh`, then the `build-firmware` skill (OTA is the easiest way to get a test build onto the device).
4. Tell the user a soak test on their network is required; host tests cannot prove radio behaviour.
