#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <uri/UriBraces.h>

#include "http_server.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "http_server_index.h"
#include "logging.h"
#include "validation.h"
#include "wifi_manager.h"

static WebServer http(HTTP_PORT);
static WebSocketsServer statusSocket(STATUS_WS_PORT);
static bool started = false;
static bool restartRequested = false;

static void sendJson(int code, JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    http.sendHeader("Access-Control-Allow-Origin", "*");
    http.send(code, "application/json", body);
}

static void sendError(int code, const char* message) {
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = message;
    sendJson(code, doc);
}

static void handleIndex() {
    http.send(200, "text/html", index_html);
}

static void handleStatus() {
    String body;
    buildStatusJson(body);
    http.sendHeader("Access-Control-Allow-Origin", "*");
    http.send(200, "application/json", body);
}

static void handleBeep() {
    logInfo("HTTP: beep");
    playSound();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["action"] = "beep";
    sendJson(200, doc);
}

static void handleBrightness() {
    uint8_t brightness;
    if (!parseBrightness(http.pathArg(0).c_str(), brightness)) {
        sendError(400, "Brightness value must be between 0 and 255.");
        return;
    }
    logInfo("HTTP: brightness %u -> %u", app.displayBrightness, brightness);
    displaySetBrightness(brightness);
    JsonDocument doc;
    doc["status"] = "ok";
    doc["variable"] = "displayBrightness";
    doc["new_value"] = String(brightness);
    sendJson(200, doc);
}

static void handleUpdateState() {
    size_t sensorId;
    const String state = http.pathArg(1);
    if (!parseSensorId(http.pathArg(0).c_str(), SENSOR_COUNT, sensorId)) {
        sendError(400, "Malformed URL. Use /config/update_state/<id>/<value>");
        return;
    }
    if (!isValidSensorState(state.c_str(), SENSOR_STATE_LEN)) {
        sendError(400, "Sensor state must be 1 to 15 letters, numbers, underscores, or hyphens.");
        return;
    }
    setSensorState(sensorId - 1, state.c_str());
    logInfo("HTTP: sensor %u manually set to %s", (unsigned)sensorId, state.c_str());
    displayRender();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["sensor_id"] = (unsigned)sensorId;
    doc["new_state"] = state;
    sendJson(200, doc);
}

static void handleBoot() {
    JsonDocument doc;
    doc["status"] = "ok";
    doc["action"] = "boot";
    sendJson(200, doc);
    // Restart from httpServerLoop() so the server can finish this response first.
    restartRequested = true;
}

static void handleNotFound() {
    logWarning("HTTP: 404 %s", http.uri().c_str());
    sendError(404, "Not found");
}

static void onStatusSocketEvent(uint8_t client, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT && length >= 6 && strncmp(reinterpret_cast<char*>(payload), "status", 6) == 0) {
        String body;
        buildStatusJson(body);
        statusSocket.sendTXT(client, body);
    }
}

void httpServerSetup() {
    if (started) {
        return;
    }
    started = true;

    http.on("/", HTTP_GET, handleIndex);
    http.on("/status", HTTP_GET, handleStatus);
    http.on("/beep", HTTP_GET, handleBeep);
    http.on("/boot", handleBoot);
    http.on(UriBraces("/config/display_brightness/{}"), HTTP_GET, handleBrightness);
    http.on(UriBraces("/config/update_state/{}/{}"), HTTP_GET, handleUpdateState);
    http.onNotFound(handleNotFound);
    http.begin();
    logInfo("HTTP server on port %u", HTTP_PORT);

    statusSocket.begin();
    statusSocket.onEvent(onStatusSocketEvent);
    logInfo("Status WebSocket on port %u", STATUS_WS_PORT);
}

void httpServerLoop() {
    if (!started) {
        return;
    }
    http.handleClient();
    statusSocket.loop();
    if (restartRequested) {
        restartDevice("requested via HTTP /boot");
    }
}

void buildStatusJson(String& out) {
    char clock[16];
    formatClock(clock, sizeof(clock));

    JsonDocument doc;
    doc["uptime"] = millis() / 1000;
    doc["current_time"] = clock;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["wifi_connected"] = app.wifiConnected;
    doc["ws_connected"] = app.hassConnected;
    // Seconds since the current WiFi session started (name kept for the UI).
    doc["wifi_last_connect_attempt"] = wifiConnectedForMs() / 1000;
    doc["wifi_rssi"] = app.wifiConnected ? WiFi.RSSI() : 0;
    doc["wifi_bssid"] = app.wifiConnected ? WiFi.BSSIDstr() : String("");
    doc["wifi_channel"] = app.wifiConnected ? WiFi.channel() : 0;
    doc["wifi_disconnects"] = wifiDisconnectCount();
    doc["wifi_last_disconnect_reason"] = wifiLastDisconnectReason();
    doc["sensor_1_state"] = app.sensorState[0];
    doc["sensor_2_state"] = app.sensorState[1];
    doc["sensor_3_state"] = app.sensorState[2];
    doc["displayBrightness"] = app.displayBrightness;
    doc["free_heap"] = ESP.getFreeHeap();

    JsonArray logs = doc["logBuffer"].to<JsonArray>();
    for (size_t i = 0; i < logCount(); i++) {
        logs.add(logEntry(i));
    }

    JsonArray matrix = doc["displayArray"].to<JsonArray>();
    for (int row = 0; row < MATRIX_HEIGHT; row++) {
        JsonArray cells = matrix.add<JsonArray>();
        for (int col = 0; col < MATRIX_WIDTH; col++) {
            cells.add(displayPixelColor(row, col));
        }
    }

    serializeJson(doc, out);
}
