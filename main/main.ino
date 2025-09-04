#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

#include "secrets.h"

// --- LED Matrix Settings ---
#define LED_PIN      23   // ESP32 pin connected to the data pin (DIN) of the matrix
#define MATRIX_WIDTH 8    // Matrix width
#define MATRIX_HEIGHT 8   // Matrix height
#define BUZZER_PIN    13  // IMPORTANT: Set this to the pin your buzzer is connected to.
#define LED_BUILTIN   2   // Onboard LED, common on many ESP32 boards

#define LED_COUNT    (MATRIX_WIDTH * MATRIX_HEIGHT)

// Initialize NeoPixel object
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- Home Assistant WebSocket client ---
using namespace websockets;
WebsocketsClient client;
WiFiUDP udp;
int hass_message_id = 1;

// Enum for colors
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

int loopInterations = 0; // blink the board led every 1500 times to show liveness
int ledState = LOW; // uses loopInterations to toggle the light
int displayBrightness = 2; // LED display brightness
int chargingRow = 1; // Used for display animation
int displayArray[MATRIX_HEIGHT][MATRIX_WIDTH];
String sensor_1_state = "unknown"; //off, disconnected, charging, unknown
String sensor_2_state = "unknown"; //off, disconnected, charging, unknown
bool wifi_connected = false;
bool ws_connected = false;
bool should_render = true; // for debugging


// --- Logging history ---
const int LOG_BUFFER_SIZE = 20;
String logBuffer[LOG_BUFFER_SIZE];


// highlight border position
// rotate a brighter pixel around the border
// to show that the system is running
int highlightIndex = 0;
int highlightX = 0;
int highlightY = 0;

// Timing control with millis()
unsigned long lastUpdate = 0;
const unsigned long interval = 1000; // 1 second

// --- Utility Functions ---
void getNextBorderPoint() {
    const int SIZE = 8;
    const int BORDER_POINTS = (SIZE * 4) - 4;
    int i = highlightIndex;

    if (i < SIZE) {
        highlightX = i;
        highlightY = 0;
    } else if (i < SIZE + SIZE - 1) {
        highlightX = SIZE - 1;
        highlightY = i - (SIZE - 1);
    } else if (i < SIZE + SIZE - 1 + SIZE - 1) {
        highlightX = (SIZE - 1) - (i - (2 * SIZE - 2));
        highlightY = SIZE - 1;
    } else {
        highlightX = 0;
        highlightY = (SIZE - 1) - (i - (3 * SIZE - 3));
    }

    highlightIndex = (highlightIndex + 1) % BORDER_POINTS;
}

void logInfo(String message){_logMessage("INFO: " + message);}
void logWarning(String message){_logMessage("WARNING: " + message);}
void logError(String message){_logMessage("ERROR: " + message);}
void logDebug(String message){_logMessage("DEBUG: " + message);}
void _logMessage(String message) {
    // Sanitize message (avoid breaking JSON)
    message.replace("\"", "' ");
    message.replace("\n", " ");

    // Get board IP
    //String ipAddress = WiFi.localIP().toString();

    if (WiFi.status() == WL_CONNECTED) {
        udp.beginPacket(LOG_UDP_IP, LOG_UDP_PORT);
        udp.write((const uint8_t*)message.c_str(), message.length());
        udp.endPacket();
    }

    Serial.println(message);

    // log history buffer
    // Shift all existing messages down by one position
    for (int i = LOG_BUFFER_SIZE - 1; i > 0; i--) {
        logBuffer[i] = logBuffer[i - 1];
    }

    // Add the new message at the beginning (index 0)
    logBuffer[0] = message;
}

void createArray8x8() {
    for (int i = 0; i < MATRIX_HEIGHT; i++) {
        for (int j = 0; j < MATRIX_WIDTH; j++) {
            displayArray[i][j] = BLACK;
        }
    }
}

void updateItem(int row, int col, Color color) {
    if (row >= 0 && row < MATRIX_HEIGHT && col >= 0 && col < MATRIX_WIDTH) {
        displayArray[row][col] = color;
    }
}

void drawBorder() {
    updateItem(0, 1, CYAN);
    updateItem(0, 2, CYAN);
    updateItem(7, 1, CYAN);
    updateItem(7, 2, CYAN);
    updateItem(0, 5, MAGENTA);
    updateItem(0, 6, MAGENTA);
    updateItem(7, 5, MAGENTA);
    updateItem(7, 6, MAGENTA);

    for (int i = 0; i < 8; i++) {
        updateItem(i, 0, CYAN);
        updateItem(i, 3, CYAN);
        updateItem(i, 4, MAGENTA);
        updateItem(i, 7, MAGENTA);
    }
}

void state_unknown(int bike, int row) {
    int colStart = (bike == 1) ? 0 : 4;
    int rowStart = (row % 2 == 0) ? 2 : 3;
    updateItem(rowStart, colStart + 1, RED);
    updateItem(rowStart, colStart + 2, RED);

    updateItem(rowStart + 2, colStart + 1, RED);
    updateItem(rowStart + 2, colStart + 2, RED);
}

void state_charging(int bike, int row) {
    int colStart = (bike == 1) ? 0 : 4;
    updateItem(row, colStart + 1, BLUE);
    updateItem(row, colStart + 2, BLUE);
}

void state_disconnected(int bike, int row) {
    int colStart = (bike == 1) ? 0 : 4;
    for (int i = 1; i < 7; i++) {
        int col = ((i + row) % 2 == 0) ? 1 : 2;
        updateItem(i, colStart + col, YELLOW);
    }
}

void noWifi(int row) {
    if (row % 2 == 0) {
        updateItem(0, 0, BLUE);
    }
    for (int r = 1; r < 8; r += 2) {
        for (int c = r; c < 8; c++) {
            updateItem(c, (7 - r), RED);
        }
    }
}

int getPixelIndex(int row, int col) {
    // Simplified from serpentine layout to standard left-to-right wiring to fix pixel scrambling.
    return (row * MATRIX_WIDTH) + col;
}

uint8_t highlightBrightness(uint8_t tone) {
    // Subtract a fixed value from the color component to make it visibly darker.
    const uint8_t brightness_change = 50;
    int16_t new_tone = (int16_t)tone - brightness_change;

    if (new_tone < 0) {
        new_tone = 0; // Prevent underflow by clamping at 0
    }

    return (uint8_t)new_tone;
}

String getEntityState(String entityId){
    HTTPClient http;

    String url = String("http://") + HASS_HOST + ":" + HASS_PORT + "/api/states/" + entityId;
    http.begin(url);

    // Add authorization header
    http.addHeader("Authorization", String("Bearer ") + HASS_TOKEN);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);

      // Parse JSON to get the state value
      // Using DynamicJsonDocument to avoid potential overflow with larger HA responses.
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      const char* state = doc["state"];
      Serial.print("Sensor state: ");
      Serial.println(state);
      http.end();
      return state;
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
      http.end();
      return "error";
    }
}

uint32_t highlightPixel(int row, int col, uint32_t rgbColor) {
    if (row == highlightY && col == highlightX) {
        // Unpack the 32-bit color into R, G, B components
        uint8_t r = (rgbColor >> 16) & 0xFF;
        uint8_t g = (rgbColor >> 8) & 0xFF;
        uint8_t b =  rgbColor & 0xFF;

        // Change brightness of each component for highlight effect
        r = highlightBrightness(r);
        g = highlightBrightness(g);
        b = highlightBrightness(b);

        // Repack the components into a 32-bit color
        return strip.Color(r, g, b);
    }
    return rgbColor;
}

void drawMatrix() {
    //Serial.println(F("--- Matrix ---"));
    for (int i = 0; i < MATRIX_HEIGHT; i++) {
        for (int j = 0; j < MATRIX_WIDTH; j++) {
            int pixelIndex = getPixelIndex(i, j);
            Color colorValue = (Color)displayArray[i][j];
            uint32_t rgbColor;

            switch (colorValue) {
                case BLACK:   rgbColor = strip.Color(0, 0, 0); break;
                case WHITE:   rgbColor = strip.Color(255, 255, 255); break;
                case RED:     rgbColor = strip.Color(255, 0, 0); break;
                case GREEN:   rgbColor = strip.Color(0, 255, 0); break;
                case BLUE:    rgbColor = strip.Color(0, 0, 255); break;
                case YELLOW:  rgbColor = strip.Color(255, 255, 0); break;
                case CYAN:    rgbColor = strip.Color(0, 255, 255); break;
                case MAGENTA: rgbColor = strip.Color(255, 0, 255); break;
                default:      rgbColor = strip.Color(0, 0, 0); break;
            }
            //Serial.print(F(" "));
            rgbColor = highlightPixel(i, j, rgbColor);
            strip.setPixelColor(pixelIndex, rgbColor);
        }
        //Serial.println();
    }
    strip.show();
    //Serial.println(F("--- End Matrix ---"));
}

// --- Home Assistant & WiFi Functions ---

void onMessage(WebsocketsMessage message) {
    logInfo("Got Message: " + message.data());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) {
        logError("deserializeJson() failed: " + String(error.c_str()));
        return;
    }

    const char* type = doc["type"];

    if (strcmp(type, "auth_required") == 0) {
        logInfo("Auth required, sending token...");
        JsonDocument auth_msg;
        auth_msg["type"] = "auth";
        auth_msg["access_token"] = HASS_TOKEN;
        String auth_str;
        serializeJson(auth_msg, auth_str);
        client.send(auth_str);
    } else if (strcmp(type, "auth_ok") == 0) {
        logInfo("Auth OK!");

        // Subscribe to sensor 1
        JsonDocument sub_msg_1;
        sub_msg_1["id"] = hass_message_id++;
        sub_msg_1["type"] = "subscribe_trigger";
        JsonObject trigger1 = sub_msg_1.createNestedObject("trigger");
        trigger1["platform"] = "state";
        trigger1["entity_id"] = SENSOR_1_ENTITY_ID;
        String sub_str_1;
        serializeJson(sub_msg_1, sub_str_1);
        client.send(sub_str_1);
        logInfo("Subscribing to " + String(SENSOR_1_ENTITY_ID));

        // Subscribe to sensor 2
        JsonDocument sub_msg_2;
        sub_msg_2["id"] = hass_message_id++;
        sub_msg_2["type"] = "subscribe_trigger";
        JsonObject trigger2 = sub_msg_2.createNestedObject("trigger");
        trigger2["platform"] = "state";
        trigger2["entity_id"] = SENSOR_2_ENTITY_ID;
        String sub_str_2;
        serializeJson(sub_msg_2, sub_str_2);
        client.send(sub_str_2);
        logInfo("Subscribing to " + String(SENSOR_2_ENTITY_ID));

    } else if (strcmp(type, "auth_invalid") == 0) {
        logError("Auth invalid. Check your HASS_TOKEN.");
    } else if (strcmp(type, "event") == 0) {
        JsonObject event = doc["event"];
        JsonObject variables = event["variables"];
        JsonObject trigger = variables["trigger"];
        const char* entity_id = trigger["entity_id"];
        const char* state = trigger["to_state"]["state"];

        if (strcmp(entity_id, SENSOR_1_ENTITY_ID) == 0) {
            sensor_1_state = state;
            logInfo("Sensor 1 state updated: " + String(sensor_1_state));
            playSound();
        } else if (strcmp(entity_id, SENSOR_2_ENTITY_ID) == 0) {
            sensor_2_state = state;
            logInfo("Sensor 2 state updated: " + String(sensor_2_state));
            Serial.println(sensor_2_state);
            playSound();
        }
    } else if (strcmp(type, "result") == 0) {
        if (doc["success"] == true) {
            logInfo("Subscription successful for ID: " + String(doc["id"].as<int>()));
        } else {
            logError("Subscription failed for ID: " + String(doc["id"].as<int>()));
            JsonObject error = doc["error"];
            logError("Error: " + String(error["message"].as<const char*>()));
        }
    }
}

void onEvent(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        logInfo("Websocket connection opened");

        ws_connected = true;
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        logInfo("Websocket connection closed");
        ws_connected = false;
    }
}

void connectToWifi() {
    logInfo("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // Don't block forever
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

void render_matrix() {
    // Clean the display array
    createArray8x8();

    if (wifi_connected) {
        drawBorder();

        if (sensor_1_state == "charging") {
            state_charging(1, chargingRow);
        } else if (sensor_1_state == "disconnected") {
            state_disconnected(1, chargingRow);
        } else if (sensor_1_state == "unknown") {
            state_unknown(1, chargingRow);
        }

        if (sensor_2_state == "charging") {
            state_charging(2, chargingRow);
        } else if (sensor_2_state == "disconnected") {
            state_disconnected(2, chargingRow);
        } else if (sensor_2_state == "unknown") {
            state_unknown(2, chargingRow);
        }

    } else {
        noWifi(chargingRow);
    }

    drawMatrix();
}

void playSound() {
    // Play two short beeps for any state change.
    // For low-level trigger active buzzer: LOW is ON, HIGH is OFF.
    
    // Beep 1
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer ON
    delay(100);                     // Beep duration 100ms
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer OFF
    delay(50);                      // Pause between beeps

    // Beep 2
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer ON
    delay(100);                     // Beep duration 100ms
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer OFF
}

void clean_display() {
    createArray8x8();
    strip.clear();
    strip.show();
    logInfo(F("Display cleared"));
}

Color stringToColor(String colorName) {
    colorName.toUpperCase();
    if (colorName == "BLACK") return BLACK;
    if (colorName == "WHITE") return WHITE;
    if (colorName == "RED") return RED;
    if (colorName == "GREEN") return GREEN;
    if (colorName == "BLUE") return BLUE;
    if (colorName == "YELLOW") return YELLOW;
    if (colorName == "CYAN") return CYAN;
    if (colorName == "MAGENTA") return MAGENTA;
    return BLACK; // Default to black if color not found
}

void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command == "render true") {
            should_render = true;
            logInfo(F("Rendering enabled"));
        } else if (command == "render false") {
            should_render = false;
            logInfo(F("Rendering disabled"));
        } else if (command == "clean") {
            clean_display();
        } else if (command.startsWith("draw ")) {
            String params = command.substring(5);
            int firstSpace = params.indexOf(' ');
            int secondSpace = params.indexOf(' ', firstSpace + 1);

            if (firstSpace > 0 && secondSpace > 0) {
                String x_str = params.substring(0, firstSpace);
                String y_str = params.substring(firstSpace + 1, secondSpace);
                String color_str = params.substring(secondSpace + 1);

                int x = x_str.toInt();
                int y = y_str.toInt();
                Color color = stringToColor(color_str);

                updateItem(y, x, color);
                drawMatrix();
                Serial.print(F("Drawing pixel at ("));
                Serial.print(x);
                Serial.print(F(", "));
                Serial.print(y);
                Serial.print(F(") with color "));
                Serial.println(color_str);
            } else {
                logError(F("Invalid draw command. Format: draw x y color"));
            }
        } else if (command == "sound") {
            logInfo(F("Playing sound..."));
            playSound();
        } else if (command == "logs") {
            Serial.println(F("--- Last Messages ---"));
            // To print in chronological order (oldest to newest)
            // With the new method, the oldest is at the end of the array.
            for (int i = LOG_BUFFER_SIZE - 1; i >= 0; i--) {
                if (logBuffer[i] != "") {
                    // Print a counter for readability
                    Serial.print((LOG_BUFFER_SIZE - i));
                    Serial.print(": ");
                    Serial.println(logBuffer[i]);
                }
            }
            Serial.println(F("--- End of Logs ---"));
        }
    }
}

// --- Main Functions ---
void setup() {
    Serial.begin(115200);

    // Initialize buzzer pin for low-level trigger module
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // Set HIGH to keep buzzer OFF

    // Initialize onboard LED
    pinMode(LED_BUILTIN, OUTPUT);
    strip.begin();
    strip.setBrightness(displayBrightness);
    strip.show(); // Initialize all pixels to 'off'
    render_matrix();
    connectToWifi();

    if (wifi_connected) {
        logInfo("Loading initial state of sensors");
        sensor_1_state = getEntityState(SENSOR_1_ENTITY_ID);
        sensor_2_state = getEntityState(SENSOR_2_ENTITY_ID);
        logInfo("Sensor 1 state: " + sensor_1_state);
        logInfo("Sensor 2 state: " + sensor_2_state);

        logInfo("Websocket connecting");
        client.onMessage(onMessage);
        client.onEvent(onEvent);
        client.connect(HASS_HOST, HASS_PORT, "/api/websocket");
        logInfo("Websocket connected");
    }
}

void loop() {

    // Toggle onboard LED as a heartbeat
    loopInterations++;
    if (loopInterations > 1500){
        loopInterations=0;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    }
 
    handleSerialCommands();

    // --- Connection Management ---
    // 1. Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connected = false; // Update our flag
        sensor_1_state = "unknown";
        sensor_2_state = "unknown";
        logWarning("WiFi disconnected, trying to reconnect...");
        connectToWifi();
        if (wifi_connected) { // If reconnection was successful, re-init the client
            logInfo("Wifi reconnected!");
            client.onMessage(onMessage);
            client.onEvent(onEvent);
            client.connect(HASS_HOST, HASS_PORT, "/api/websocket");
        }
    }

    // 2. If WiFi is on, manage WebSocket connection
    if (wifi_connected) {
        if (!ws_connected) {
            sensor_1_state = "unknown";
            sensor_2_state = "unknown";
            logWarning("Websocket disconnected, trying to reconnect...");
            // Attempt to reconnect. The client.poll() below will handle the connection process.
            client.connect(HASS_HOST, HASS_PORT, "/api/websocket");
        }
        client.poll(); // This must be called to process messages and maintain connection
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
