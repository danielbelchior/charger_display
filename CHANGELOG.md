# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). The firmware has no version numbers yet; entries are grouped under *Unreleased* until the first tagged release.

## Unreleased

### Fixed

- WiFi drops on Ubiquiti/UniFi networks:
  - Modem sleep is off, so beacons and group rekeys are no longer missed (`BEACON_TIMEOUT`, `GROUP_KEY_UPDATE_TIMEOUT`).
  - Full channel scan and connect-by-signal pick the strongest AP instead of the first one heard.
  - The core's auto-reconnect is off and a single state machine owns reconnects, ending the "sta is connecting" race that could wedge the driver.
  - Credentials are no longer rewritten to flash on every retry (`WiFi.persistent(false)`).
- Silently dead Home Assistant sessions after an AP roam: HA `ping` every 30 s, reconnect after 75 s without traffic.
- Reboot loops during AP or HA restarts: reboot only after 5 min without WiFi or 10 min without HA (was 30 s).
- A rejected HA token (`auth_invalid`) is reported as such instead of as a closed socket, and retried every 60 s.
- `/boot` answers the HTTP request before restarting.

### Added

- Disconnect diagnostics: reason names, drop counter, RSSI, BSSID, and channel in the log, the web UI, and `/status` (`wifi_rssi`, `wifi_bssid`, `wifi_channel`, `wifi_disconnects`, `wifi_last_disconnect_reason`, `sensor_3_state`).
- Restart reason kept in RTC memory and logged on the next boot together with the hardware reset reason.
- Redesigned control panel: self-contained (no CDN), live matrix preview, demo mode (`?demo` or `file://`), stale-data banner, works by IP or any hostname.
- Optional settings in `secrets.h`: `DEVICE_HOSTNAME`, `NTP_SERVER`, `TIMEZONE`, `DOOR_SENSOR_ENTITY_IDS`, `ENABLE_OTA`, `OTA_PASSWORD`.
- PlatformIO build (`platformio.ini`) with `usb` and `ota` environments, pinned to Arduino-ESP32 3.2.0, used by CI including an OTA-enabled build. Guide in `docs/platformio.md`.
- `pio run -e ota -t upload` reads the OTA password from `secrets.h` (`scripts/pio_ota_auth.py`).
- Host unit tests for the WiFi reconnect policy, input validation, and the log ring (`tests/host`, `scripts/test.sh`), run in CI.
- Documentation: architecture, API, WiFi troubleshooting, development, domain glossary (`CONTEXT.md`), decision records (`docs/adr/`), contributor guide, and agent instructions (`AGENTS.md`).
- Code style config: `.editorconfig`, `.clang-format`, `scripts/format.sh`.

### Changed

- Default timezone is Europe/Amsterdam (`CET-1CEST,M3.5.0,M10.5.0/3`). Set `TIMEZONE` in `secrets.h` to change it.
- Sensor 3 is the air compressor: on a state change it scrolls a green `ON` or red `OFF` across the matrix, in addition to the existing beeps. The web UI labels it "Air compressor".
- New matrix layout: each charger is a battery icon (green fill while charging, empty blue outline when disconnected, yellow when unknown) and the middle columns show the heartbeat or the WiFi / Home Assistant fault. During an outage the chargers keep their last known state instead of switching to `unknown`.
- Partition table is `min_spiffs.csv` (two 1.9 MB app slots) instead of the 1.25 MB default, to leave room for OTA. The first flash after updating must be over USB.
- Firmware split into modules (`config`, `app_state`, `logging`, `buzzer`, `display`, `display_frame`, `wifi_policy`, `wifi_manager`, `hass`, `validation`, `http_server`, `ota`). `declarations.h` and `util.*` are gone.
- Time comes from SNTP via `configTzTime` instead of the NTPClient library.
- Logging uses printf-style `logInfo/logWarning/logError/logDebug`.
- REST input (brightness, sensor id, sensor state) is validated before use.
