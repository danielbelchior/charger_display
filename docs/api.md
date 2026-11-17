# API

All endpoints are served on port 80 and send `Access-Control-Allow-Origin: *`.

| Method | Path | Description |
| --- | --- | --- |
| GET | `/` | Web UI |
| GET | `/status` | Status JSON (see below) |
| GET | `/beep` | Plays the default beep |
| GET/POST | `/boot` | Replies, then reboots |
| GET | `/config/display_brightness/{0-255}` | Sets the LED brightness (not persisted) |
| GET | `/config/update_state/{1-3}/{state}` | Overrides a sensor state until the next Home Assistant update. `state` is 1-15 characters from `[A-Za-z0-9_-]` |

Errors return a 4xx code with `{"status":"error","message":"..."}`.

## Status document

`GET /status` returns the same JSON that the status WebSocket on port 81 sends after each `status` text message. The web UI polls it once per second.

| Key | Type | Meaning |
| --- | --- | --- |
| `firmware_version` | string | Release tag such as `v2026.11.17.1`, a `git describe` string for other builds, or `dev` |
| `uptime` | int | Seconds since boot |
| `current_time` | string | `HH:MM:SS` local time, or `+HH:MM:SS` uptime before NTP sync |
| `ip_address` | string | Station IP |
| `wifi_connected` | bool | WiFi link up with an IP |
| `ws_connected` | bool | Home Assistant authenticated and subscribed |
| `wifi_last_connect_attempt` | int | Seconds since the current WiFi session started (0 when offline). The name is historical |
| `wifi_rssi` | int | Signal strength in dBm (0 when offline) |
| `wifi_bssid` | string | MAC address of the AP in use |
| `wifi_channel` | int | WiFi channel |
| `wifi_disconnects` | int | WiFi drops since boot |
| `wifi_last_disconnect_reason` | string | Last reason, e.g. `BEACON_TIMEOUT` |
| `sensor_1_state` ... `sensor_3_state` | string | Current sensor states |
| `displayBrightness` | int | 0-255 |
| `free_heap` | int | Bytes |
| `logBuffer` | string[] | Recent log lines, newest first |
| `displayArray` | int[8][8] | `0xRRGGBB` per pixel as currently shown |
