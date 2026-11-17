#pragma once

// Central place for build-time configuration. Values the user must provide
// live in secrets.h; everything else has a default here that secrets.h may
// override by defining the macro first.

#include <Arduino.h>
#include "secrets.h"

// --- Hardware ---------------------------------------------------------------
constexpr uint8_t LED_PIN = 23;
constexpr uint8_t BUZZER_PIN = 13;  // active-low buzzer module
constexpr uint8_t MATRIX_WIDTH = 8;
constexpr uint8_t MATRIX_HEIGHT = 8;
constexpr uint16_t LED_COUNT = MATRIX_WIDTH * MATRIX_HEIGHT;
constexpr uint8_t DEFAULT_BRIGHTNESS = 20;

// --- Network services -------------------------------------------------------
#ifndef DEVICE_HOSTNAME
#define DEVICE_HOSTNAME "charger"
#endif

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

// POSIX TZ string. Default: Europe/Amsterdam (CET, CEST from the last Sunday of
// March 02:00 to the last Sunday of October 03:00). UTC is "UTC0".
#ifndef TIMEZONE
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t STATUS_WS_PORT = 81;

// --- WiFi -------------------------------------------------------------------
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t WIFI_RETRY_MIN_MS = 1000;
constexpr uint32_t WIFI_RETRY_MAX_MS = 30000;
// Reboot only after a long outage; short AP reboots or roams must not reset us.
constexpr uint32_t WIFI_RESTART_AFTER_MS = 5UL * 60UL * 1000UL;
constexpr int8_t WIFI_WEAK_RSSI_DBM = -75;
constexpr uint32_t WIFI_WEAK_RSSI_LOG_INTERVAL_MS = 10UL * 60UL * 1000UL;

// --- Home Assistant ---------------------------------------------------------
constexpr uint32_t HASS_RETRY_MS = 10000;
constexpr uint32_t HASS_AUTH_RETRY_MS = 60000;  // after auth_invalid
constexpr uint32_t HASS_AUTH_TIMEOUT_MS = 10000;
constexpr uint32_t HASS_PING_INTERVAL_MS = 30000;
// No traffic (including pong replies) for this long means the TCP session is dead.
constexpr uint32_t HASS_IDLE_TIMEOUT_MS = 75000;
constexpr uint16_t HASS_HTTP_TIMEOUT_MS = 4000;
constexpr uint32_t HASS_RESTART_AFTER_MS = 10UL * 60UL * 1000UL;

// Door/presence sensors that trigger a short beep when they turn "on".
// Override in secrets.h with your own braced list, or { nullptr } to disable.
// clang-format off
#ifndef DOOR_SENSOR_ENTITY_IDS
#define DOOR_SENSOR_ENTITY_IDS {                      \
    "binary_sensor.door_backyard_contact",            \
    "binary_sensor.door_0_coffee_contact",            \
    "binary_sensor.door_0_kitchen_contact",           \
    "binary_sensor.presence_backyard_door_occupancy"  \
}
#endif
// clang-format on

// --- Display / UI -----------------------------------------------------------
constexpr uint32_t DISPLAY_FRAME_MS = 1000;
// Scrolling ON/OFF text for the air compressor (sensor 3).
constexpr uint32_t DISPLAY_MESSAGE_STEP_MS = 90;
constexpr uint32_t DISPLAY_MESSAGE_PASSES = 2;

// --- Logging ----------------------------------------------------------------
constexpr size_t LOG_BUFFER_SIZE = 30;
constexpr size_t LOG_LINE_LEN = 100;

// --- OTA --------------------------------------------------------------------
#ifndef ENABLE_OTA
#define ENABLE_OTA 0
#endif
