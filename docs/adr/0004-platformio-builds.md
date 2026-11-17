# 0004. Firmware builds use PlatformIO

Status: Accepted
Date: 2026-11-17

## Context

Arduino IDE builds depend on whatever core and library versions are installed on each machine, so CI and local builds disagreed. A Docker image with arduino-cli was tried, but it was heavy (several hundred MB per image, blocked registries on some networks) and gave no help with flashing or OTA.

## Decision

- `platformio.ini` is the single build definition: board `esp32dev`, the pioarduino platform pinned to a release (Arduino-ESP32 3.2.0), the `min_spiffs.csv` partition table, and `lib_deps` with version ranges.
- Two environments: `usb` (default, serial upload) and `ota` (espota to `charger.local`). `scripts/pio_ota_auth.py` reads the OTA password from `main/secrets.h`, so it is written down only once.
- CI runs `pio run -e usb` and an OTA-enabled `pio run -e ota`, after the host tests (`scripts/test.sh`, plain C++ compiler).
- `scripts/pio_version.py` compiles the version (CI release tag or `git describe`) into `FIRMWARE_VERSION`.
- `src_dir = main`, so the sketch still opens in the Arduino IDE.

## Consequences

- Changing the core or a library means editing `platformio.ini`; see `docs/platformio.md`.
- The official `platformio/espressif32` platform is on Arduino-ESP32 2.x; moving to it would be a core downgrade and needs a WiFi soak test.
- The partition table differs from the Arduino IDE default. The first flash after switching must go over USB, and Arduino IDE users must pick *Minimal SPIFFS* to stay compatible with OTA.
