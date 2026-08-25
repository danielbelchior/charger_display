#include "declarations.h"
#include "logging.h"
#include "display.h"
#include "hass.h"
#include "http_server.h"
#include <ESPmDNS.h>
// OTA
// #include "ota.h"
// Utility
#include "util.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

// --- Global Variable Definitions ---
// The 'extern' declarations in declarations.h tell other files that these variables exist.
// Here is where they are actually created and memory is allocated for them.

// NeoPixel
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t displayBrightness = 20;
uint8_t displayArray[MATRIX_HEIGHT][MATRIX_WIDTH];

// Home Assistant
WebsocketsClient client;
int hass_message_id = 1;
char sensor_1_state[16] = "unknown";
char sensor_2_state[16] = "unknown";
char sensor_3_state[16] = "unknown";
bool ws_connected = false;
bool refreshHassStates = false;

// WiFi
WiFiServer server(HTTP_PORT);
WiFiUDP udp;
bool wifi_connected = false;
unsigned long wifiLastConnectAttempt = 0;

// State & Timing
int chargingRow = 1;
int highlightIndex = 0;
int highlightX = 0;
int highlightY = 0;
bool should_render = true;
unsigned long lastUpdate = 0;
const unsigned long interval = 1000; // 1 second

// Connection retry timing
const unsigned long WIFI_RETRY_INITIAL_DELAY = 5000;
const unsigned long WIFI_RETRY_MAX_DELAY = 60000;
const unsigned long HASS_RETRY_DELAY = 10000;
unsigned long nextWifiRetryAt = 0;
unsigned long wifiRetryDelay = WIFI_RETRY_INITIAL_DELAY;
unsigned long nextHassRetryAt = 0;
bool httpServerStarted = false;

// Logging
char logBuffer[LOG_BUFFER_SIZE][100] = {0};
//bool ota_in_progress = false;

// NTP
NTPClient timeClient(udp, NTP_SERVER, 0, 3600000); // UTC, update every 1h


// --- WiFi and Utility Functions ---

void connectToWifi() {
    logInfo("Connecting to WiFi...");
    logInfo("SSID: " + String(WIFI_SSID));

    WiFi.setHostname("charger");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    nextWifiRetryAt = millis() + wifiRetryDelay;
    wifiRetryDelay = min(wifiRetryDelay * 2, WIFI_RETRY_MAX_DELAY);
}

void copySensorState(char* destination, const String& state) {
    state.toCharArray(destination, 16);
}

void handleWifiConnected() {
    wifi_connected = true;
    wifiLastConnectAttempt = millis();
    wifiRetryDelay = WIFI_RETRY_INITIAL_DELAY;
    logInfo("WiFi connected!");
    logInfo("IP address: " + WiFi.localIP().toString());
    timeClient.update();

    if (!MDNS.begin("charger")) {
        logError("Error starting mDNS");
    } else {
        logInfo("mDNS responder started: charger.local");
    }

    logInfo("Loading initial state of sensors");
    getEntitiesState();

    logInfo("Websocket connecting");
    setupHass();
    nextHassRetryAt = millis() + HASS_RETRY_DELAY;

    if (!httpServerStarted) {
        setupHttpServer();
        httpServerStarted = true;
    }

    render_matrix();
}

void handleWifiConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifi_connected) {
            handleWifiConnected();
        }
        return;
    }

    if (wifi_connected) {
        wifi_connected = false;
        ws_connected = false;
        strcpy(sensor_1_state, "unknown");
        strcpy(sensor_2_state, "unknown");
        strcpy(sensor_3_state, "unknown");
        logWarning("WiFi disconnected.");
        render_matrix();
    }

    if ((long)(millis() - nextWifiRetryAt) >= 0) {
        logWarning("WiFi reconnect attempt.");
        connectToWifi();
    }
}



// --- Main Functions ---

void setup() {
    Serial.begin(115200);
    // setupOTA("charger");

    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // Keep buzzer OFF

    // Play buzzer sound
    playSound(2, 20, 50);


    setupDisplay();
    render_matrix(); // Initial render

    connectToWifi();
    playSound(3, 20, 50);

    // NTP setup
    timeClient.begin();
    playSound(1, 50, 400);
}

void getEntitiesState() {
    copySensorState(sensor_1_state, getEntityState(SENSOR_1_ENTITY_ID));
    copySensorState(sensor_2_state, getEntityState(SENSOR_2_ENTITY_ID));
    copySensorState(sensor_3_state, getEntityState(SENSOR_3_ENTITY_ID));
    logInfo("Sensor 1 state: " + String(sensor_1_state));
    logInfo("Sensor 2 state: " + String(sensor_2_state));
    logInfo("Sensor 3 state: " + String(sensor_3_state));
}

void loop() {

    // OTA DISABLED
    // Handle OTA updates
    // handleOTA();
    // if (ota_in_progress){
    //     return;
    // }


    handleHttpRequests();
    handleWebSocket();

    // --- Connection Management ---
    handleWifiConnection();

    if (wifi_connected) {
        if (!ws_connected) {
            if ((long)(millis() - nextHassRetryAt) >= 0) {
                logWarning("Websocket disconnected, trying to reconnect...");
                setupHass();
                nextHassRetryAt = millis() + HASS_RETRY_DELAY;
            }
        }
        loopHass(); // Process WebSocket messages
        if (refreshHassStates) {
            refreshHassStates = false;
            getEntitiesState();
        }
    }

    // --- Timed Display Update ---
    unsigned long now = millis();
    if (now - lastUpdate >= interval) {
        lastUpdate = now;

        chargingRow++;
        if (chargingRow > 6) chargingRow = 1;

        getNextBorderPoint();

        if (should_render) {
            render_matrix();
        }
    }
}