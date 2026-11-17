---
name: build-firmware
description: Build, test, and flash the charger_display ESP32 firmware with PlatformIO. Use when asked to compile, build, verify, test, flash, or OTA-update firmware in this repository, run the host unit tests, or check formatting.
---

# Build, test, and flash the firmware

Full guide: [docs/platformio.md](../../../docs/platformio.md).

## 1. Host unit tests (always)

```sh
scripts/test.sh            # all tests
scripts/test.sh wifi       # tests whose name contains "wifi"
CXXFLAGS="-fsanitize=address,undefined" scripts/test.sh
```

- Builds `tests/host/*.cpp` with `main/wifi_policy.cpp` and `main/validation.cpp` using `$CXX` (default `c++`), `-Wall -Wextra -Werror`.
- A new pure source file must be added to the compile line in `scripts/test.sh`.

## 2. Formatting

```sh
scripts/format.sh --check   # CLANG_FORMAT=/path/to/clang-format if not on PATH
```

Fix with `scripts/format.sh`. Never format `main/http_server_index.h`.

## 3. Firmware compile (PlatformIO)

```sh
test -f main/secrets.h || cp main/secrets_template.h main/secrets.h
pio run                      # env "usb"
```

- The first run downloads the platform, toolchain, and libraries (several hundred MB) into `~/.platformio`. Ask first if the user said not to download or has limited bandwidth. Do not install PlatformIO without permission.
- CI also builds the OTA variant: add `#define ENABLE_OTA 1` to `main/secrets.h`, run `pio run -e ota`, then restore the file if it was the user's.
- If you created `main/secrets.h` from the template only to compile, delete it afterwards.
- Check the size line: flash use must stay below 100 % of the 1.9 MB app slot.

## 4. Flash (only when the user asks)

Flashing changes a physical device; confirm first.

```sh
pio run -t upload -t monitor               # USB
pio run -e ota -t upload                   # WiFi; needs ENABLE_OTA 1 on the device
pio run -e ota -t upload --upload-port <ip>
```

The OTA password comes from `main/secrets.h` (or the `OTA_PASSWORD` environment variable) via `scripts/pio_ota_auth.py`. Never print it.

## 5. Report

State exactly which steps ran and their results. If PlatformIO or the network was unavailable, say the firmware was **not compiled**. Hardware behaviour (WiFi, HA, LEDs) is never proven by these steps.
