# Architecture

The firmware is a plain Arduino sketch in `main/`. Everything runs cooperatively in `loop()` on one task. The only exception is the WiFi event callback, which just sets flags.

## Modules

| Module | Responsibility |
| --- | --- |
| `config.h` | Pins, timings, and optional settings, with defaults that `secrets.h` can override |
| `secrets.h` | User values: WiFi, Home Assistant host, token, and entity IDs (gitignored) |
| `app_state.*` | Shared state (`app`): sensor states, connection flags, brightness. `restartDevice()` stores the restart reason in RTC memory so it survives the reboot |
| `logging.*` | `logInfo/logWarning/logError/logDebug` (printf-style). Logs go to Serial and a 30-line `LogRing` shown in the web UI |
| `log_ring.h` | Fixed-size ring of log lines. Pure, host tested |
| `buzzer.*` | Blocking beep patterns |
| `display_frame.*` | Builds the 8x8 frame (batteries, status column, OTA) from charger states, link state, and the animation step. Pure, host tested |
| `display.*` | Feeds `app` into `composeFrame()`, advances the animation step while connected, and drives the NeoPixel strip |
| `wifi_policy.*` | WiFi reconnect state machine (Waiting / Connecting / Connected, backoff, core retry, restart). Pure, host tested |
| `wifi_manager.*` | Adapter: radio settings, driver events into `WifiPolicy`, its actions back to the driver, signal monitoring |
| `validation.*` | Parsing of REST input (brightness, sensor id, sensor state) and the HA on-state check. Pure, host tested |
| `hass.*` | Home Assistant WebSocket (live updates and heartbeat) plus REST (initial state) |
| `http_server.*` | Web UI, REST API (port 80), and status WebSocket (port 81) |
| `http_server_index.h` | Generated web UI (do not edit; see development.md) |
| `ota.*` | ArduinoOTA, compiled only when `ENABLE_OTA` is 1 |
| `main.ino` | `setup()` and `loop()` wiring the modules together |

Dependencies point one way: `main.ino` uses all modules, and modules use `config.h`, `app_state.h`, and `logging.h`. No module includes `main.ino` symbols. The pure modules (`wifi_policy`, `validation`, `log_ring`, `display_frame`) include only the C/C++ standard library, so `tests/host/` can build them without Arduino ([ADR 0005](adr/0005-pure-logic-host-tests.md)).

## Main loop

```
loop()
 ├─ otaLoop()            returns early while an update is flashing
 ├─ wifiLoop()           -> Connected / Disconnected transitions
 │    └─ on first connect: SNTP, mDNS, HTTP server, OTA
 ├─ hassLoop()           connect, authenticate, subscribe, ping, refresh
 ├─ httpServerLoop()     web requests and status WebSocket
 ├─ superviseHomeAssistant()  reboot after 10 min without HA
 └─ displayLoop()        animation once per second, ON/OFF scroll every 90 ms
```

## WiFi connection handling

`WifiPolicy` (`wifi_policy.cpp`) decides and `wifi_manager.cpp` carries out the decisions. The core's auto-reconnect is off because it races with our own `WiFi.begin()` calls.

- **Waiting**: wait for the retry time, then call `WiFi.begin()`.
- **Connecting**: success when the IP arrives. If the attempt fails (disconnect event) or takes 20 s, retry with exponential backoff from 1 s to 30 s.
- **Connected**: on a disconnect, log the reason code and retry at once.
- The ESP32 core retries the first disconnect of each boot on its own, even with auto-reconnect off. The state machine waits for that attempt instead of starting a second one.
- Late `ASSOC_LEAVE` events (from our own `disconnect()`) are ignored unless connected.
- After 5 minutes offline, the device reboots.

The radio settings that matter for UniFi are explained in [wifi-troubleshooting.md](wifi-troubleshooting.md).

## Home Assistant connection handling

1. Open `ws://HASS_HOST:HASS_PORT/api/websocket` with a new client object for each connection.
2. Answer `auth_required` with the token. On `auth_ok`, subscribe (`subscribe_trigger`) to the three sensors and the door sensors.
3. Fetch the current value of each sensor over REST (4 s timeout), because triggers only report changes.
4. Every 30 s, send an HA `ping`. If nothing arrives for 75 s, drop and reconnect. This catches TCP sessions that died silently during an AP roam.
5. Retry every 10 s. After `auth_invalid`, retry every 60 s.

## Restart policy

| Condition | Action |
| --- | --- |
| WiFi offline for 5 min | Reboot |
| WiFi up but Home Assistant not ready for 10 min | Reboot |
| `GET/POST /boot` | Reboot |

Each restart logs its reason on the next boot (`Boot: previous restart requested because: ...`), together with the hardware reset reason (power-on, brownout, watchdog, and so on).
