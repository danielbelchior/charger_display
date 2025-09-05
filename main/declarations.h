#ifndef DECLARATIONS_H
#define DECLARATIONS_H

// --- Libraries ---
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "secrets.h"

// --- Namespaces ---
using namespace websockets;

// --- LED Matrix Settings ---
#define LED_PIN      23
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 8
#define BUZZER_PIN    13
#define LED_BUILTIN   2
#define LED_COUNT    (MATRIX_WIDTH * MATRIX_HEIGHT)
#define HTTP_PORT    80

// --- Logging ---
#define LOG_BUFFER_SIZE 20

// --- Enum for colors ---
enum Color {
    BLACK = 0,
    WHITE,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA
};

// --- Extern Global Variables ---
// By declaring these as 'extern', we tell other .cpp files that these variables exist somewhere else (in main.ino).
// This allows them to be shared across files.

// OTA update
extern bool ota_in_progress;

// NeoPixel
extern Adafruit_NeoPixel strip;
extern int displayBrightness;
extern int displayArray[MATRIX_HEIGHT][MATRIX_WIDTH];

// Home Assistant
extern WebsocketsClient client;
extern int hass_message_id;
extern String sensor_1_state;
extern String sensor_2_state;
extern bool ws_connected;

// WiFi
extern WiFiUDP udp;
extern bool wifi_connected;
extern WiFiServer server;

// State & Timing
extern int chargingRow;
extern int highlightIndex;
extern int highlightX;
extern int highlightY;
extern bool should_render;
extern unsigned long lastUpdate;
extern const unsigned long interval;

// Logging
extern String logBuffer[LOG_BUFFER_SIZE];

// --- Function Prototypes for main.ino ---
// Functions that are in main.ino but called from other files
void playSound(int beeps = 2, int delayBetweenBeep = 50, int duration = 100);
void connectToWifi();


#endif // DECLARATIONS_H
