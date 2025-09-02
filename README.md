# E-Bike Charger Status Display

This project uses an ESP32 and an 8x8 NeoPixel LED matrix to display the charging status of two e-bikes. It connects to a Home Assistant instance via WebSockets to get real-time status from two specified sensor entities.

## Features

-   **Dual Display**: The 8x8 matrix is split vertically to show the status of two separate chargers.
-   **Clear Visual States**:
    -   **Charging**: An animated blue bar rises to indicate charging.
    -   **Disconnected/Off**: A scrolling yellow pattern.
    -   **Unknown**: A static red indicator.
    -   **No WiFi**: A large red 'X' across the display.
-   **Live Indicator**: A single brighter pixel continuously rotates around the border to show the system is active and running.
-   **Home Assistant Integration**: Connects directly to your Home Assistant's WebSocket API for real-time, local updates.

## Hardware Requirements

-   An ESP32 development board (e.g., ESP32-WROOM-32).
-   An 8x8 WS2812B (NeoPixel) RGB LED matrix.
-   A 5V power supply capable of providing at least 2A (recommended for the matrix).
-   Jumper wires.

### Wiring

Connect the components as follows. **Important**: Ensure a common ground between the ESP32 and the matrix power supply.

| ESP32 Pin | LED Matrix Pin |
| :-------- | :------------- |
| `GPIO 23` | `DIN` (Data In) |
| `GND`     | `GND`          |
| `5V` / `VIN` | `5V`           |

*Note: The `LED_PIN` is defined as `23` in `main.ino`. You can change this to any other suitable GPIO pin on your ESP32.*

It is highly recommended to power the LED matrix from an external 5V power supply, not directly from the ESP32's 5V pin, as the matrix can draw significant current.

## Software & Setup

### 1. Arduino IDE Setup

1.  **Install Arduino IDE**: Download and install the latest version from the official Arduino website.

2.  **Add ESP32 Board Support**:
    -   Open Arduino IDE and go to `File` > `Preferences` (or `Arduino IDE` > `Settings...` on macOS).
    -   In the "Additional Board Manager URLs" field, add the following URL:
        ```
        https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
        ```
    -   Click "OK".
    -   Go to `Tools` > `Board` > `Boards Manager...`.
    -   Search for "esp32" and install the package by **Espressif Systems**.

3.  **Select Your Board**:
    -   Go to `Tools` > `Board` and select your specific ESP32 board model (e.g., "ESP32 Dev Module").

### 2. Install Required Libraries

You can install these libraries via the Arduino Library Manager. Go to `Sketch` > `Include Library` > `Manage Libraries...`.

Search for and install the following:

-   `Adafruit NeoPixel` by Adafruit
-   `ArduinoWebsockets` by Markus Sattler
-   `ArduinoJson` by Benoit Blanchon

### 3. Configure Project Secrets

The project requires credentials for your WiFi network and Home Assistant instance.

1.  Find the `secrets_template.h` file in the project directory.
2.  Create a copy of this file and rename it to `secrets.h`.
3.  Open `secrets.h` and fill in the required values:

```cpp
// secrets.h
#pragma once

// CONFIG WIFI
#define WIFI_SSID "YourWifiSSID"
#define WIFI_PASSWORD "YourWifiPassword"

// CONFIG HOME ASSISTANT
#define HASS_HOST "homeassistant.local" // Or your HA IP address
#define HASS_PORT 8123
#define HASS_TOKEN "Your_Long_Lived_Access_Token"

// SENSORS
#define SENSOR_1_ENTITY_ID "sensor.ebike_1_status"
#define SENSOR_2_ENTITY_ID "sensor.ebike_2_status"
```

**How to get a Home Assistant Token:**
1.  In your Home Assistant UI, click on your user profile in the bottom-left corner.
2.  Scroll down to the "Long-Lived Access Tokens" section.
3.  Click "Create Token", give it a name (e.g., "ESP32 Charger Display"), and copy the generated token. **You will only see this token once.**

**Sensor States:**
The code expects the sensor entities to have states like `charging`, `disconnected`, or `off`. If your smart plugs or sensors provide different states (e.g., numeric power values), you may need to create a Template Sensor in Home Assistant to translate those values into the expected string states.

## Compilation and Upload

1.  Connect your ESP32 to your computer via a USB cable.
2.  In the Arduino IDE, go to `Tools` > `Port` and select the correct serial port for your ESP32.
3.  Click the "Upload" button (the arrow icon) to compile the sketch and upload it to the board.
4.  If the upload fails, some ESP32 boards require you to hold down the `BOOT` button on the board when the IDE starts the upload process.

Once uploaded, the ESP32 will attempt to connect to your WiFi. You can open the Serial Monitor (`Tools` > `Serial Monitor`, set to `115200` baud) to see debug messages and connection status.
