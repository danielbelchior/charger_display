# Domain context

Shared vocabulary for this repository. Use these terms in code, logs, docs, and reviews. Decisions behind the design are in [docs/adr/](docs/adr/).

## The device

| Term | Meaning |
| --- | --- |
| **Display** | The whole device: ESP32-WROOM-32, 8x8 WS2812B **matrix**, and active-low **buzzer**. Hostname `charger` by default |
| **Matrix** | The 8x8 LED grid. Left half (columns 0-3) is **charger 1**, right half (columns 4-7) is **charger 2**. Wired row by row, not serpentine |
| **Frame** | One full image pushed to the matrix. A new frame is drawn every `DISPLAY_FRAME_MS` (1 s) |
| **Heartbeat pixel** | The single bright pixel that moves one step around the border each frame. Proves the main loop is alive |
| **Pattern** | What a half or the whole matrix shows for a given state (charging bar, zig-zag, staircase, checkerboard). Listed in the README |
| **Brightness** | NeoPixel brightness 0-255, default 20. Set from the web UI, not persisted |

## Home Assistant

| Term | Meaning |
| --- | --- |
| **HA** | Home Assistant, reached at `HASS_HOST:HASS_PORT` with a long-lived access **token** |
| **Sensor** | One of three HA entities in `SENSOR_ENTITY_IDS` (from `secrets.h`). Sensors 1 and 2 are the **chargers**; sensor 3 only makes sounds |
| **Sensor id** | 1-based index used by the REST API (`/sensor/{1..3}/...`). Code arrays are 0-based (`index = id - 1`) |
| **Sensor state** | The HA state string, stored in `app.sensorState`. The display understands `charging`, `disconnected`, and anything else as `unknown`. Max 15 characters from `[A-Za-z0-9_-]` |
| **Door sensor** | Entities in `DOOR_SENSOR_ENTITY_IDS`. Turning **on** gives a short beep. They are not shown on the matrix |
| **On state** | `on` or `true`, case-insensitive (`isOnState`) |
| **Manual override** | Setting a sensor state through `/sensor/{id}/{state}`. Lasts until HA sends the next change or the next refresh |
| **HA ready** | WebSocket open, `auth_ok` received, and subscriptions sent. `app.hassConnected` mirrors this |
| **Refresh** | REST `GET /api/states/<entity>` for each sensor after connecting, because triggers only report changes |
| **HA ping / idle timeout** | HA-level `ping` every 30 s; no traffic for 75 s means the TCP session is dead and is dropped |

## WiFi

| Term | Meaning |
| --- | --- |
| **WifiPolicy** | The pure reconnect state machine in `main/wifi_policy.*`. Decides; never touches the driver |
| **Adapter** | `main/wifi_manager.cpp`. Feeds driver events to the policy and carries out its actions (`Begin`, `Disconnect`, `Restart`) |
| **Waiting / Connecting / Connected** | The policy states. Waiting = backoff timer running; Connecting = `WiFi.begin()` issued; Connected = got IP and link up |
| **Attempt** | One `WiFi.begin()` until success, a disconnect event, or the 20 s connect timeout |
| **Backoff** | Delay before the next attempt: 1 s doubling to 30 s, reset on connect and on a fresh link loss |
| **Core retry** | The ESP32 Arduino core retries the first disconnect of each boot by itself, even with auto-reconnect off. The policy waits for it once instead of racing it |
| **Disconnect reason** | The 802.11 / ESP-IDF reason code of a drop (for example 200 `BEACON_TIMEOUT`). Named by `wifiReasonName()` |
| **Late ASSOC_LEAVE** | Reason 8 arriving while not Connected. It is the echo of our own `disconnect()` and is ignored |
| **Drop** | A loss of a Connected link. Counted in `wifi_disconnects` |
| **Offline** | Not Connected. 5 min offline reboots the display |

## Operations

| Term | Meaning |
| --- | --- |
| **Restart reason** | Text passed to `restartDevice()`, kept in RTC memory and logged on the next boot next to the hardware **reset reason** |
| **Supervision** | Reboot rules: WiFi offline 5 min, or WiFi up but HA not ready for 10 min |
| **Event log** | The 30-line in-memory ring (`LogRing`) shown in the web UI and `/status`. Lost on reboot |
| **Control panel** | The web UI. Source `control-panel/index.html`, served from `main/http_server_index.h` |
| **Status WebSocket** | Port 81. Pushes the `/status` JSON to the control panel |
| **Demo mode** | Control panel opened from `file://` or with `?demo`: animates sample data, no device needed |
| **Host tests** | Unit tests in `tests/host` for the Arduino-free modules, built with a normal C++ compiler |
