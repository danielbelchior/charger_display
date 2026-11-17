# Contributing

Thanks for helping. This is a small hobby firmware, so the process is light, but a few rules keep the display reliable.

## Before you start

- Read [CONTEXT.md](CONTEXT.md) for the vocabulary and [docs/adr/](docs/adr/) for decisions that should not be undone by accident (no core auto-reconnect, modem sleep off, self-contained web UI, PlatformIO builds, pure logic with host tests).
- For anything larger than a fix, open an issue first and describe the change.

## Setup

```sh
cp main/secrets_template.h main/secrets.h     # fill in your values; never commit it
git config core.hooksPath .githooks          # regenerates the web UI header on commit
```

The hook needs `html-minifier` (`npm install -g html-minifier`). See [docs/development.md](docs/development.md) for details.

## Checks to run

| Check | Command | Needs |
| --- | --- | --- |
| Host unit tests | `scripts/test.sh` | Any C++17 compiler |
| Formatting | `scripts/format.sh --check` | `clang-format` |
| Firmware build | `pio run` | [PlatformIO](docs/platformio.md) |

CI runs the host tests, a format check, and two firmware builds (with and without OTA). Every successful push to `main` also creates a dated release tag (see [docs/development.md](docs/development.md#ci)). Changes to WiFi or Home Assistant handling also need a test on real hardware: leave the display running for at least a day and check `wifi_disconnects` in `/status` and the event log.

## Code guidelines

- C++17, 4 spaces, 120 columns, formatted by `.clang-format`. `main/http_server_index.h` is generated; do not format or edit it by hand.
- One module per `.h`/`.cpp` pair in `main/`. Expose `<name>Setup()` / `<name>Loop()` and wire them in `main.ino`.
- Never block the loop for long. The display, web server, and HA heartbeat share it.
- Logic that decides something (timers, retries, parsing) goes in an Arduino-free module with tests in `tests/host/`. Pass the time in as `now`; do not call `millis()` there.
- Compare times with unsigned subtraction (`now - start >= timeout`) so `millis()` wraparound is safe.
- Tunables go in `main/config.h`; use an `#ifndef` default when users may override them in `secrets.h`.
- Log with `logInfo/logWarning/logError("Module: ...")` from the main task only. Never log secrets (WiFi password, HA token, OTA password).
- Comments explain *why* (a hardware quirk, a UniFi behaviour), not what the code does.

## Changing the web UI

Edit `control-panel/index.html` and preview it with `?demo` or from `file://`. The hook copies it into `main/http_server_index.h`. Keep it self-contained: no CDN links, no web fonts. New status fields need changes in `http_server.cpp`, the UI, and [docs/api.md](docs/api.md).

## Commits and pull requests

- Small, focused commits with a short imperative subject (`fix hass reconnect after auth_invalid`).
- Add a line under *Unreleased* in [CHANGELOG.md](CHANGELOG.md) for anything a user would notice.
- Update docs in the same pull request when behaviour, the API, or configuration changes.
