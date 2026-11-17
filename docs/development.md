# Development

## Building and flashing

The firmware builds with PlatformIO. [platformio.ini](../platformio.ini) pins the board, the ESP32 core, the partition table, and the libraries.

```sh
cp main/secrets_template.h main/secrets.h   # once, then edit
pio run                                     # compile
pio run -t upload -t monitor                # flash over USB and watch the log
pio run -e ota -t upload                    # flash over WiFi (ENABLE_OTA 1)
```

The full guide covers installing PlatformIO, the first USB flash, OTA setup, firewalls and VLANs, and troubleshooting: [platformio.md](platformio.md).

## Host unit tests

The Arduino-free modules (`wifi_policy.*`, `validation.*`, `log_ring.h`, `display_frame.*`) have unit tests in `tests/host/` that run on your computer with any C++17 compiler. No ESP32 toolchain is needed.

```sh
scripts/test.sh                 # build and run all tests
scripts/test.sh backoff         # only tests whose name contains "backoff"
CXXFLAGS="-fsanitize=address,undefined" scripts/test.sh
```

`tests/host/test.h` is a small built-in harness (`TEST_CASE`, `CHECK`, `CHECK_EQ`, `CHECK_STR`). Add a test file there, and add any new pure `.cpp` to the compile line in `scripts/test.sh`.

## Formatting

```sh
scripts/format.sh            # format in place
scripts/format.sh --check    # fail if anything needs formatting
```

Uses `.clang-format` (4 spaces, 120 columns). Set `CLANG_FORMAT=/path/to/clang-format` if it is not on your `PATH`. `.editorconfig` covers the other file types.
## Web UI

`control-panel/index.html` is the source. The firmware serves the minified copy in `main/http_server_index.h`. The pre-commit hook regenerates it when the HTML is staged:

```sh
git config core.hooksPath .githooks
npm install -g html-minifier   # needed by the hook
```

The UI uses relative URLs, so it works from `charger.local`, the IP address, or any other hostname. It is a single self-contained file (no CDN, no web fonts), so it also loads on networks without internet access.

To preview design changes without a device, open `control-panel/index.html` directly in a browser (`file://`) or add `?demo` to the URL. The page then animates sample data instead of connecting to the display.

If `html-minifier` isn't installed, you can paste the HTML unminified between the `R"rawliteral(` and `)rawliteral"` markers in `main/http_server_index.h`. The firmware serves it the same way, it just takes more flash.

## Adding a module

1. Create `main/<name>.h` and `main/<name>.cpp`. Arduino compiles every `.cpp` in `main/`.
2. If it decides something (timers, retries, parsing), put that logic in an Arduino-free file and test it in `tests/host/` (see [ADR 0005](adr/0005-pure-logic-host-tests.md)).
3. Put tunables in `config.h`, with an `#ifndef` default when the user may override them in `secrets.h`.
4. Expose `<name>Setup()` and `<name>Loop()` and call them from `main.ino`. Never block for long: the display, web server, and HA heartbeat share the loop.
5. Log with `logInfo("Module: ...")` from the main task only.

## CI

`.github/workflows/c-cpp.yml` runs the host tests, then compiles with PlatformIO twice: `pio run -e usb` with the template secrets, and `pio run -e ota` with OTA enabled.

## Log receiver (optional)

`http-server/` contains a small aiohttp app that receives UDP log lines and streams them in a browser. The firmware doesn't send UDP logs by default.
