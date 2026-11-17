# Building and flashing with PlatformIO

The firmware builds with [PlatformIO](https://platformio.org). `platformio.ini` in the repository root sets the board, ESP32 core, partition table, and libraries, so every machine and CI build the same thing.

| Setting | Value |
| --- | --- |
| Platform | [pioarduino](https://github.com/pioarduino/platform-espressif32) `54.03.20` = Arduino-ESP32 **3.2.0** (ESP-IDF 5.4.1) |
| Board | `esp32dev` (ESP32-WROOM-32) |
| Partitions | `min_spiffs.csv`: two 1.9 MB app slots for OTA, 190 KB SPIFFS (unused) |
| Libraries | ArduinoJson 7, Adafruit NeoPixel, WebSockets (links2004), ArduinoWebsockets (gilmaimon) |
| Environments | `usb` (default): serial upload. `ota`: upload over WiFi with espota |

The official `platformio/espressif32` platform still ships Arduino-ESP32 2.x. That is why the platform line points to a pioarduino release.

## 1. Install PlatformIO

Pick one:

```sh
pipx install platformio          # or: pip install --user platformio
brew install platformio          # macOS with Homebrew
```

Or install the *PlatformIO IDE* extension in VS Code, which bundles the same `pio` command.

USB-serial driver: most ESP32 dev boards use a CP210x or CH340 chip. macOS and Linux include drivers for both. Windows may need the driver from the chip vendor.

## 2. Configure

```sh
cp main/secrets_template.h main/secrets.h
```

Fill in WiFi, Home Assistant host and token, and the sensor entity IDs. To allow WiFi updates later, also enable OTA **before the first USB flash**:

```c
#define ENABLE_OTA 1
#define OTA_PASSWORD "pick-a-long-password"
```

Without `OTA_PASSWORD`, anyone on your network can flash the display.

## 3. Compile

```sh
pio run
```

The first run downloads the platform, the Xtensa toolchain, and the libraries into `~/.platformio` (several hundred MB). Later runs reuse them and only rebuild what changed.

Output: `.pio/build/usb/firmware.bin`. The last lines show flash and RAM use. Flash use must stay below 100 % of the 1.9 MB app slot.

Other useful commands:

| Command | Purpose |
| --- | --- |
| `pio run -t clean` | Delete build output |
| `pio run -v` | Show full compiler commands |
| `pio pkg update` | Update libraries within the ranges in `platformio.ini` |
| `pio run -t size` | Size report |

## 4. Flash over USB

Connect the board and run:

```sh
pio run -t upload
pio device monitor              # serial log at 115200 baud; Ctrl+C to quit
```

`pio run -t upload -t monitor` does both in one step.

- If several serial devices are attached, choose one with `--upload-port /dev/cu.usbserial-0001` (macOS), `/dev/ttyUSB0` (Linux), or `COM5` (Windows). `pio device list` shows the candidates.
- If the upload stays at `Connecting....`, hold the **BOOT** button on the board until writing starts.
- Upload errors at 921600 baud usually mean a poor cable. Set `upload_speed = 460800` in `[env:usb]`.

**First flash after switching from the Arduino IDE:** the partition table changes from the IDE default (1.25 MB app) to `min_spiffs.csv`. A partition table can only be written over USB, so the first PlatformIO flash must be a USB flash. Settings in NVS are kept. If the device boot-loops afterwards, run `pio run -t erase` once and flash again.

After boot, the serial log shows lines like:

```
Boot: firmware v2026.11.17.1
Boot: reset reason power-on
WiFi: connected to 78:45:58:AA:BB:CC ch 6, RSSI -61 dBm, IP 192.168.1.50
OTA: ready
```

`OTA: ready` only appears when `ENABLE_OTA` is 1.

## 5. Update over WiFi (OTA)

Requirements: the running firmware was built with `ENABLE_OTA 1`, the device is online, and your computer can reach it.

```sh
pio run -e ota -t upload
```

This builds the `ota` environment and sends it with espota to `charger.local`, port 3232.

- **Password:** taken from `OTA_PASSWORD` in `main/secrets.h` by `scripts/pio_ota_auth.py`. An `OTA_PASSWORD` environment variable overrides it: `OTA_PASSWORD=... pio run -e ota -t upload`.
- **Other hostname or no mDNS:** pass the IP address. You can find it in the serial log, under `ip_address` in `http://<device>/status`, or in your router's client list:

  ```sh
  pio run -e ota -t upload --upload-port 192.168.1.50
  ```

  If you changed `DEVICE_HOSTNAME`, also change `upload_port` in `[env:ota]`.

During the update the matrix shows a yellow checkerboard, and the buzzer beeps at the start, at every 10 %, and at the end. The device then reboots into the new firmware. A failed transfer leaves the old firmware running, because OTA writes to the inactive app slot.

### How espota connects

espota sends an invitation over UDP to port 3232. The ESP32 then opens a TCP connection **back to your computer** on a random port between 10000 and 60000 and downloads the image. This matters when:

- **A firewall runs on your computer.** Allow incoming connections for Python, or fix the port and allow only that port:

  ```ini
  ; platformio.ini, [env:ota]
  upload_flags =
      --port=3232
      --host_port=40000
  ```

- **The display is on a separate IoT VLAN** (common with UniFi). The firewall must allow UDP 3232 from your computer to the display, and TCP from the display to your computer on the host port. Otherwise the upload stops at `Waiting for device...`.

### OTA troubleshooting

| Message | Cause | Fix |
| --- | --- | --- |
| `No response from device` | OTA not compiled in, wrong address, or UDP 3232 blocked | Check the boot log for `OTA: ready`; use the IP instead of `.local`; check VLAN rules |
| `Authentication Failed` | Password differs from the one in the running firmware | Use the password the device was built with (env var override), then fix `secrets.h` |
| Stops at `Waiting for device...` | Device cannot connect back to your computer | Allow the host port in your firewall or VLAN rules (see above) |
| `charger.local` does not resolve | mDNS does not cross VLANs or is blocked | Use `--upload-port <ip>` |
| Device log shows `OTA: error 2` (connect) or `OTA: error 3` / `4` (receive / end) | Weak WiFi or the transfer was interrupted | Retry; check `wifi_rssi` in `/status` |
| Build fails with `The program size (...) is greater than maximum allowed` | Firmware larger than the app slot | Check `pio run -t size`; the partition table must stay `min_spiffs.csv` |

To turn OTA off again, set `ENABLE_OTA 0` (or remove the line) and do one more upload. That upload can still go over OTA.

## Versions

Every build has a firmware version. `scripts/pio_version.py` sets it in this order:

1. The `FIRMWARE_VERSION` environment variable. CI release builds set it to the new tag.
2. `git describe --tags --match 'v*' --always --dirty`:
   - on a release tag: `v2026.11.17.1`
   - 3 commits after a tag: `v2026.11.17.1-3-gabc1234`
   - with uncommitted changes: `...-dirty` (`main/secrets.h` is ignored by git and does not count)
   - before the first tag: the short commit hash
3. `dev` if git is not available. Arduino IDE builds always show `dev`.

The version appears in the boot log (`Boot: firmware v2026.11.17.1`), as `firmware_version` in `/status`, under *Firmware* in the web UI, and as the `version` TXT record of the mDNS `_http._tcp` service.

CI creates the tags (see [development.md](development.md#ci)). To flash a release:

```sh
git fetch --tags
git checkout v2026.11.17.1
pio run -t upload               # or: pio run -e ota -t upload
git checkout main
```

## Changing versions

- **ESP32 core:** change the `platform` URL to another [pioarduino release](https://github.com/pioarduino/platform-espressif32/releases). The release title names the Arduino-ESP32 version. Then run `pio run -t clean` and build again.
- **Libraries:** edit the ranges in `lib_deps`. Pin an exact version (for example `bblanchon/ArduinoJson@7.4.1`) to freeze it.
- WiFi behaviour depends on the core version. After a core upgrade, let the display run for a day and watch `wifi_disconnects` in `/status`.

## Arduino IDE

`main/main.ino` still opens in the Arduino IDE (folder `main`). Install the four libraries and the *esp32 by Espressif* core 3.2.x, choose *ESP32 Dev Module*, and set *Partition Scheme* to **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** so the layout matches PlatformIO. With OTA enabled, the device also shows up as a network port in the IDE.
