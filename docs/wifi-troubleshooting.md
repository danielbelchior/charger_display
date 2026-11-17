# WiFi troubleshooting (Ubiquiti / UniFi)

## Symptom

The display randomly switches to the red "no WiFi" pattern, or to the blue "no Home Assistant" pattern, on a network with UniFi access points. Older firmware also rebooted after 30 seconds of any outage.

## Causes and firmware fixes

| Cause | What happened | Fix in `wifi_manager.cpp` / `hass.cpp` |
| --- | --- | --- |
| **Modem sleep** (ESP32 default) | The radio sleeps between DTIM beacons and can miss beacons or the group key rekey. UniFi then drops the client with `BEACON_TIMEOUT` (200) or `GROUP_KEY_UPDATE_TIMEOUT` (16). | `WiFi.setSleep(false)` |
| **Joining the wrong AP** | The default fast scan joins the first matching AP it hears, often a distant one on a low channel. A weak link gets kicked by UniFi's minimum-RSSI setting or drops on its own. | `WIFI_ALL_CHANNEL_SCAN` plus `WIFI_CONNECT_AP_BY_SIGNAL` picks the strongest AP |
| **Reconnect race** | The core's auto-reconnect and the sketch's own `WiFi.begin()` ran at the same time ("sta is connecting"), which could leave the driver stuck. | Auto-reconnect is off. A single state machine with backoff owns reconnects |
| **Dead TCP after a roam or AP restart** | WiFi came back, but the Home Assistant socket was silently dead, so no updates arrived. | HA `ping` every 30 s. If nothing arrives for 75 s, reconnect |
| **Reboot loop** | The old watchdog rebooted after 30 s without WiFi or HA, for example during HA restarts or AP firmware updates. | Reboot only after 5 min without WiFi, or 10 min without HA |
| **Flash wear** | `WiFi.begin()` rewrote the credentials to flash on every retry. | `WiFi.persistent(false)` |

## Recommended UniFi settings

The ESP32 supports only 2.4 GHz, WPA2-PSK or WPA3-SAE (no 802.11r), and no band steering. Apply these to the SSID the display uses. A dedicated IoT SSID is easiest.

| Setting (UniFi Network) | Value | Why |
| --- | --- | --- |
| Band | 2.4 GHz only (or enable *Enhanced IoT Connectivity*) | Prevents band steering from pushing the client toward 5 GHz |
| Security | WPA2 (or WPA2/WPA3 transitional) | WPA3-only with PMF required fails on some ESP32 builds |
| Fast Roaming (802.11r) | Off | Not supported; can break association |
| BSS Transition | Off | Roam requests are ignored and can end in a deauth |
| Minimum RSSI | Off, or below -80 dBm | Otherwise a client at -75 dBm keeps getting kicked |
| Multicast Enhancement | Off | Can break mDNS (`charger.local`) |
| Group Rekey Interval | Default (3600 s) or off | Short intervals make rekey timeouts more likely |
| DTIM period (2.4 GHz) | 1 or 3 (default) | Large values make IoT clients drop with beacon timeouts |
| 2.4 GHz channel width | 20 MHz | Better range and fewer collisions |
| Channel | Fixed 1, 6, or 11 | Auto channel changes disconnect every client on that radio |

If the device sits between two APs, consider *Lock to AP* for its client entry in UniFi, so it always joins the closer one.

## Reading the diagnostics

The web UI shows `WiFi Signal: -61 dBm (2 drops, last: BEACON_TIMEOUT)`. The full data is in `/status` (`wifi_rssi`, `wifi_bssid`, `wifi_channel`, `wifi_disconnects`, `wifi_last_disconnect_reason`). Logs look like:

```
WiFi: connected to 78:45:58:AA:BB:CC ch 6, RSSI -61 dBm, IP 192.168.1.50
WiFi: disconnected after 5321s, reason 200 (BEACON_TIMEOUT)
WiFi: weak signal -79 dBm from 78:45:58:AA:BB:CC
Boot: reset reason brownout
```

| Reason | Usual meaning on UniFi |
| --- | --- |
| 200 `BEACON_TIMEOUT` | Weak signal, interference, or modem sleep |
| 16 `GROUP_KEY_UPDATE_TIMEOUT` | Missed group rekey (sleep, or a short rekey interval) |
| 2 `AUTH_EXPIRE` / 4 `ASSOC_EXPIRE` | AP removed an idle client, or an AP restarted |
| 8 `ASSOC_LEAVE` | Local disconnect (normal during a retry) |
| 15 / 204 handshake timeout | Wrong password, or a WPA3/PMF mismatch |
| 201 `NO_AP_FOUND` | SSID not visible on 2.4 GHz, or out of range |
| Reset reason `brownout` | Power supply sags at WiFi TX peaks. Use a better 5 V supply or a 470 µF capacitor near the ESP32 |

A BSSID that keeps changing means the device is moving between APs. Lock it to one AP, or lower the transmit power of the far AP.
