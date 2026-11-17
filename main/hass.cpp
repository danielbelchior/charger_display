#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <HTTPClient.h>
#include <memory>

#include "hass.h"
#include "app_state.h"
#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "logging.h"
#include "validation.h"

using websockets::WebsocketsClient;
using websockets::WebsocketsEvent;
using websockets::WebsocketsMessage;

enum class Phase : uint8_t { Disconnected, Authenticating, Ready };

static const char* const DOOR_SENSORS[] = DOOR_SENSOR_ENTITY_IDS;

// A fresh client per connection: reusing one instance after a broken TCP
// session is unreliable in ArduinoWebsockets.
static std::unique_ptr<WebsocketsClient> ws;
static Phase phase = Phase::Disconnected;
static int nextMessageId = 1;
static uint32_t nextConnectAt = 0;
static uint32_t connectStartedAt = 0;
static uint32_t lastReceivedAt = 0;
static uint32_t lastPingAt = 0;
static uint32_t notReadySince = 0;
static bool socketClosed = false;
static bool authRejected = false;
static bool refreshPending = false;
static bool wifiWasDown = true;

static bool isDoorSensor(const char* entityId) {
    for (const char* door : DOOR_SENSORS) {
        if (door && strcmp(entityId, door) == 0) {
            return true;
        }
    }
    return false;
}

static void sendJson(JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    ws->send(payload);
}

static void subscribe(const char* entityId) {
    JsonDocument doc;
    doc["id"] = nextMessageId++;
    doc["type"] = "subscribe_trigger";
    JsonObject trigger = doc["trigger"].to<JsonObject>();
    trigger["platform"] = "state";
    trigger["entity_id"] = entityId;
    sendJson(doc);
    logInfo("HASS: subscribing to %s", entityId);
}

static void sendPing() {
    JsonDocument doc;
    doc["id"] = nextMessageId++;
    doc["type"] = "ping";
    sendJson(doc);
}

static void fetchEntityState(size_t index) {
    const char* entityId = SENSOR_ENTITY_IDS[index];
    char url[160];
    snprintf(url, sizeof(url), "http://%s:%d/api/states/%s", HASS_HOST, (int)HASS_PORT, entityId);

    HTTPClient http;
    http.setConnectTimeout(HASS_HTTP_TIMEOUT_MS);
    http.setTimeout(HASS_HTTP_TIMEOUT_MS);
    if (!http.begin(url)) {
        logError("HASS: bad URL %s", url);
        setSensorState(index, "error");
        return;
    }
    http.addHeader("Authorization", String("Bearer ") + HASS_TOKEN);

    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        logError("HASS: GET %s failed (%d)", entityId, status);
        setSensorState(index, "error");
        http.end();
        return;
    }

    JsonDocument filter;
    filter["state"] = true;
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, http.getString(), DeserializationOption::Filter(filter));
    http.end();
    if (error) {
        logError("HASS: bad state JSON for %s: %s", entityId, error.c_str());
        setSensorState(index, "error");
        return;
    }
    setSensorState(index, doc["state"] | "error");
    logInfo("HASS: sensor %u = %s", (unsigned)(index + 1), app.sensorState[index]);
}

static void handleTrigger(const char* entityId, const char* toState, const char* fromState) {
    if (isDoorSensor(entityId)) {
        if (isOnState(toState) && !isOnState(fromState)) {
            logInfo("HASS: door opened: %s", entityId);
            playSound(1, 50, 100);
        }
        return;
    }

    for (size_t i = 0; i < SENSOR_COUNT; i++) {
        if (strcmp(entityId, SENSOR_ENTITY_IDS[i]) != 0) {
            continue;
        }
        setSensorState(i, toState);
        logInfo("HASS: sensor %u -> %s", (unsigned)(i + 1), app.sensorState[i]);
        displayRender();
        if (i < 2) {
            playSound();
        } else {
            // Sensor 3 is the air compressor.
            const bool on = isOnState(toState);
            if (on) {
                playSound(3, 50, 100);
            } else {
                playSound(1, 50, 300);
            }
            // Attribute-only updates arrive with an unchanged state; no text for those.
            if (strcmp(toState, fromState) != 0) {
                displayShowMessage(on ? "ON" : "OFF", on ? GREEN : RED);
            }
        }
        return;
    }
}

static const JsonDocument& messageFilter() {
    static JsonDocument filter;
    if (filter.isNull()) {
        filter["type"] = true;
        filter["id"] = true;
        filter["success"] = true;
        filter["error"]["message"] = true;
        JsonObject trigger = filter["event"]["variables"]["trigger"].to<JsonObject>();
        trigger["entity_id"] = true;
        trigger["to_state"]["state"] = true;
        trigger["from_state"]["state"] = true;
    }
    return filter;
}

static void onMessage(WebsocketsMessage message) {
    lastReceivedAt = millis();

    JsonDocument doc;
    const DeserializationError error =
        deserializeJson(doc, message.data(), DeserializationOption::Filter(messageFilter()));
    if (error) {
        logError("HASS: bad message JSON: %s", error.c_str());
        return;
    }

    const char* type = doc["type"] | "";

    if (strcmp(type, "auth_required") == 0) {
        JsonDocument auth;
        auth["type"] = "auth";
        auth["access_token"] = HASS_TOKEN;
        sendJson(auth);
    } else if (strcmp(type, "auth_ok") == 0) {
        logInfo("HASS: authenticated");
        phase = Phase::Ready;
        app.hassConnected = true;
        lastPingAt = millis();
        for (size_t i = 0; i < SENSOR_COUNT; i++) {
            subscribe(SENSOR_ENTITY_IDS[i]);
        }
        for (const char* door : DOOR_SENSORS) {
            if (door) {
                subscribe(door);
            }
        }
        // subscribe_trigger only reports changes, so load current values once.
        refreshPending = true;
    } else if (strcmp(type, "auth_invalid") == 0) {
        logError("HASS: auth rejected, check HASS_TOKEN");
        authRejected = true;
    } else if (strcmp(type, "event") == 0) {
        JsonObject trigger = doc["event"]["variables"]["trigger"];
        handleTrigger(trigger["entity_id"] | "", trigger["to_state"]["state"] | "unknown",
                      trigger["from_state"]["state"] | "");
    } else if (strcmp(type, "result") == 0 && !(doc["success"] | false)) {
        logError("HASS: request %d failed: %s", doc["id"] | 0, doc["error"]["message"] | "?");
    }
}

static void onEvent(WebsocketsEvent event, String) {
    if (event == WebsocketsEvent::ConnectionClosed) {
        socketClosed = true;
    } else if (event == WebsocketsEvent::GotPong || event == WebsocketsEvent::GotPing) {
        lastReceivedAt = millis();
    }
}

static void dropConnection(const char* reason, uint32_t retryMs) {
    if (phase != Phase::Disconnected) {
        logWarning("HASS: disconnected (%s)", reason);
    }
    if (ws) {
        ws->close();
        ws.reset();
    }
    if (phase == Phase::Ready) {
        notReadySince = millis();
    }
    phase = Phase::Disconnected;
    app.hassConnected = false;
    nextConnectAt = millis() + retryMs;
    displayRender();
}

static void startConnection() {
    logInfo("HASS: connecting to %s:%d", HASS_HOST, (int)HASS_PORT);
    ws.reset(new WebsocketsClient());
    ws->onMessage(onMessage);
    ws->onEvent(onEvent);
    socketClosed = false;
    authRejected = false;
    nextMessageId = 1;
    connectStartedAt = millis();
    lastReceivedAt = connectStartedAt;

    if (!ws->connect(HASS_HOST, HASS_PORT, "/api/websocket")) {
        ws.reset();
        logWarning("HASS: connection failed");
        nextConnectAt = millis() + HASS_RETRY_MS;
        return;
    }
    phase = Phase::Authenticating;
}

void hassLoop() {
    if (!app.wifiConnected) {
        if (phase != Phase::Disconnected) {
            dropConnection("WiFi down", 0);
        }
        wifiWasDown = true;
        return;
    }
    if (wifiWasDown) {
        // The HA restart timer only counts time during which WiFi was up.
        wifiWasDown = false;
        notReadySince = millis();
    }

    if (phase == Phase::Disconnected) {
        if ((int32_t)(millis() - nextConnectAt) >= 0) {
            startConnection();
        }
        return;
    }

    ws->poll();

    const uint32_t now = millis();
    // HA closes the socket right after auth_invalid, so check that first.
    if (authRejected) {
        dropConnection("auth rejected", HASS_AUTH_RETRY_MS);
        return;
    }
    if (socketClosed) {
        dropConnection("socket closed", HASS_RETRY_MS);
        return;
    }
    if (phase == Phase::Authenticating && now - connectStartedAt >= HASS_AUTH_TIMEOUT_MS) {
        dropConnection("auth timeout", HASS_RETRY_MS);
        return;
    }
    if (now - lastReceivedAt >= HASS_IDLE_TIMEOUT_MS) {
        // Typical after an AP roam: the TCP session is gone but nothing told us.
        dropConnection("no traffic, assuming dead link", 0);
        return;
    }
    if (phase == Phase::Ready && now - lastPingAt >= HASS_PING_INTERVAL_MS) {
        lastPingAt = now;
        sendPing();
    }

    if (refreshPending && phase == Phase::Ready) {
        refreshPending = false;
        for (size_t i = 0; i < SENSOR_COUNT; i++) {
            fetchEntityState(i);
        }
        displayRender();
    }
}

bool hassIsConnected() {
    return phase == Phase::Ready;
}

uint32_t hassDisconnectedForMs() {
    return phase == Phase::Ready ? 0 : millis() - notReadySince;
}
