// Copy this file to secrets.h (gitignored) and fill in your values.
#pragma once

// --- WiFi ---
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

// --- Home Assistant ---
// An IP address is more reliable than a .local name.
#define HASS_HOST "homeassistant.local"
#define HASS_PORT 8123
// Long-lived access token (HA profile page -> Security).
#define HASS_TOKEN "TOKEN"

// --- Sensors ---
// Sensors 1 and 2 are the chargers shown on the matrix (left and right half).
// Expected states: "charging", "disconnected", "unknown".
#define SENSOR_1_ENTITY_ID "sensor.sensor_1"
#define SENSOR_2_ENTITY_ID "sensor.sensor_2"
// Sensor 3 only beeps: 3 short beeps when it turns "on", one long beep otherwise.
#define SENSOR_3_ENTITY_ID "sensor.sensor_3"

// --- Optional overrides (defaults live in config.h) ---
// #define NTP_SERVER "pool.ntp.org"
// #define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
// #define DEVICE_HOSTNAME "charger"
// #define DOOR_SENSOR_ENTITY_IDS { "binary_sensor.front_door_contact" }
// OTA: enable before the first USB flash, then update with `pio run -e ota -t upload`
// (docs/platformio.md). The PlatformIO ota env reads OTA_PASSWORD from this file.
// #define ENABLE_OTA 1
// #define OTA_PASSWORD "change-me"
