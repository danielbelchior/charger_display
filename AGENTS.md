# AGENTS.md

Instructions for AI coding agents working in this repository. Humans: see [CONTRIBUTING.md](CONTRIBUTING.md).

## Project

ESP32 firmware (Arduino sketch in `main/`) that shows two e-bike charger states from Home Assistant on an 8x8 WS2812B matrix, with a buzzer and a web control panel. Hardware: ESP32-WROOM-32 (`esp32:esp32:esp32`), LEDs on GPIO 23, active-low buzzer on GPIO 13.

Read first: [CONTEXT.md](CONTEXT.md) (glossary), [docs/architecture.md](docs/architecture.md) (modules, loop, connection handling), [docs/adr/](docs/adr/) (decisions you must not undo silently).

## Commands

| Task | Command | Notes |
| --- | --- | --- |
| Host unit tests | `scripts/test.sh [name-filter]` | Needs only a C++17 compiler. Fast; run after every logic change |
| Format | `scripts/format.sh` / `scripts/format.sh --check` | clang-format, config in `.clang-format` |
| Firmware build | `pio run` | PlatformIO, config in `platformio.ini`. Needs `main/secrets.h` (copy the template). First run downloads several hundred MB |
| Flash | `pio run -t upload` (USB), `pio run -e ota -t upload` (WiFi) | Only when the user asks. See [docs/platformio.md](docs/platformio.md) |

`platformio.ini` is the only build definition ([ADR 0004](docs/adr/0004-platformio-builds.md)). Change core or library versions there, not in the IDE. Do not install PlatformIO or other toolchains without asking. If PlatformIO or the network is unavailable, say the firmware was not compiled instead of claiming it builds.

## Layout

```
main/                 Firmware. Arduino compiles every .cpp here
  wifi_policy.*       Pure WiFi reconnect state machine (host tested)
  validation.*        Pure input parsing / HA on-state (host tested)
  log_ring.h          Pure event-log ring buffer (host tested)
  display_frame.*     Pure 8x8 frame composer: batteries + status column (host tested)
  wifi_manager.*      Adapter: ESP32 WiFi driver <-> WifiPolicy
  hass.*              Home Assistant WebSocket + REST
  http_server.*       Web UI, REST API (80), status WebSocket (81)
  http_server_index.h GENERATED from control-panel/index.html
  config.h            Pins, timings, overridable defaults
  secrets.h           User secrets, gitignored. Never create, print, or commit real values
tests/host/           Host unit tests + tiny harness (test.h)
control-panel/        Web UI source (single self-contained HTML file)
docs/                 Architecture, API, WiFi troubleshooting, development, ADRs
platformio.ini        Build definition: board, core, partitions, libraries, usb/ota envs
scripts/              test.sh, format.sh, pio_ota_auth.py (OTA password for espota)
http-server/          Optional UDP log receiver (Python), not part of the firmware
```

## Rules

- **WiFi**: never call `WiFi.begin()`, `WiFi.reconnect()`, `setAutoReconnect(true)`, or `setSleep(true)` outside `wifi_manager.cpp`. Change reconnect behaviour in `WifiPolicy` and add a test in `tests/host/test_wifi_policy.cpp` ([ADR 0001](docs/adr/0001-own-wifi-reconnect.md), [0002](docs/adr/0002-wifi-modem-sleep-off.md)).
- **Pure modules** (`wifi_policy.*`, `validation.*`, `log_ring.h`, `display_frame.*`) include only standard C/C++ headers. No `Arduino.h`, no `millis()`; time is a `uint32_t now` parameter ([ADR 0005](docs/adr/0005-pure-logic-host-tests.md)).
- **Timing**: compare with unsigned subtraction (`now - start >= timeout`); it survives `millis()` wraparound. Never `now >= deadline`.
- **Loop**: everything runs cooperatively in `loop()`. No `delay()` beyond a few ms, no blocking network calls without a timeout. The WiFi event callback only sets flags.
- **Web UI**: edit `control-panel/index.html`, then sync `main/http_server_index.h` (see the `update-web-ui` skill). No CDN, fonts, or external assets ([ADR 0003](docs/adr/0003-self-contained-web-ui.md)). Do not hand-edit the header otherwise.
- **Display**: patterns use only the full-intensity `Color` palette; the hardware dims the whole display, never single LEDs. Change patterns in `display_frame.cpp`, cover them in `tests/host/test_display_frame.cpp`, and keep the web UI demo frame and the README pattern table in step.
- **API changes**: keep `http_server.cpp`, `control-panel/index.html`, and `docs/api.md` in agreement.
- **Config**: new tunables go in `config.h`; user-overridable ones get an `#ifndef` default and a commented entry in `secrets_template.h`.
- **Secrets**: never log or echo `WIFI_PASSWORD`, `HASS_TOKEN`, or `OTA_PASSWORD`.
- **Style**: C++17, 4 spaces, 120 columns, `.clang-format`. Comments explain why, not what.
- **Docs**: update README / `docs/` / `CHANGELOG.md` (*Unreleased*) when behaviour, API, or configuration changes.

## Verifying changes

1. `scripts/test.sh` passes (it builds with `-Wall -Wextra -Werror`).
2. `scripts/format.sh --check` is clean.
3. `pio run` compiles, when PlatformIO is available. Otherwise state that the firmware was not compiled.
4. WiFi or HA changes cannot be proven off-device. Say so, and suggest a soak test on the real network (watch `wifi_disconnects` and the event log in `/status`).

## Repo skills and droids

- `.factory/skills/build-firmware`: test, build, and flash workflow with PlatformIO.
- `.factory/skills/update-web-ui`: edit and sync the control panel.
- `.factory/skills/debug-wifi-drops`: diagnose disconnects from logs and `/status`.
- `.factory/droids/esp32-firmware-reviewer`: read-only reviewer for firmware diffs.
