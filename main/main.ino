#include "declarations.h"
#include "logging.h"
#include "display.h"
#include "hass.h"
#include "serial_commands.h"
#include "http_server.h"

// --- Global Variable Definitions ---
// The 'extern' declarations in declarations.h tell other files that these variables exist.
// Here is where they are actually created and memory is allocated for them.

// NeoPixel
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int displayBrightness = 2;
int displayArray[MATRIX_HEIGHT][MATRIX_WIDTH];

// Home Assistant
WebsocketsClient client;
int hass_message_id = 1;
String sensor_1_state = "unknown";
String sensor_2_state = "unknown";
bool ws_connected = false;

// WiFi
WiFiServer server(HTTP_PORT);
WiFiUDP udp;
bool wifi_connected = false;

// State & Timing
int chargingRow = 1;
int highlightIndex = 0;
int highlightX = 0;
int highlightY = 0;
bool should_render = true;
unsigned long lastUpdate = 0;
const unsigned long interval = 1000; // 1 second

// Logging
String logBuffer[LOG_BUFFER_SIZE];

// Onboard LED heartbeat
int loopIterations = 0;
int ledState = LOW;


// --- WiFi and Utility Functions ---

void connectToWifi() {
    logInfo("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        logInfo("WiFi connected!");
        logInfo("IP address: " + WiFi.localIP().toString());
        wifi_connected = true;
    } else {
        logError("Failed to connect to WiFi.");
        wifi_connected = false;
    }
}

void playSound() {
    // Play two short beeps for any state change.
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
}


// --- Main Functions ---

void setup() {
    Serial.begin(115200);

    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // Keep buzzer OFF

    // Initialize onboard LED
    pinMode(LED_BUILTIN, OUTPUT);

    setupDisplay();
    render_matrix(); // Initial render
    
    connectToWifi();

    if (wifi_connected) {
        logInfo("Loading initial state of sensors");
        getEntitiesState();

        logInfo("Websocket connecting");
        setupHass();
        logInfo("Websocket connected");

        setupHttpServer();
    }
}

void getEntitiesState() {
    sensor_1_state = getEntityState(SENSOR_1_ENTITY_ID);
    sensor_2_state = getEntityState(SENSOR_2_ENTITY_ID);
    logInfo("Sensor 1 state: " + sensor_1_state);
    logInfo("Sensor 2 state: " + sensor_2_state);
}

void loop() {
    // Toggle onboard LED as a heartbeat
    loopIterations++;
    if (loopIterations > 1500){
        loopIterations=0;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    }

    handleSerialCommands();
    handleHttpRequests();

    // --- Connection Management ---
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connected = false;
        ws_connected = false; // Also reset websocket status
        sensor_1_state = "unknown";
        sensor_2_state = "unknown";
        logWarning("WiFi disconnected, trying to reconnect...");
        connectToWifi();
        if (wifi_connected) {
            logInfo("Wifi reconnected!");
            getEntitiesState();
            // Re-init the HASS connection
            setupHass();
        }
    }

    if (wifi_connected) {
        if (!ws_connected) {
            sensor_1_state = "unknown";
            sensor_2_state = "unknown";
            logWarning("Websocket disconnected, trying to reconnect...");
            // Attempt to reconnect.
            client.connect(HASS_HOST, HASS_PORT, "/api/websocket");
            getEntitiesState();
        }
        loopHass(); // Process WebSocket messages
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