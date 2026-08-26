#include "hass.h"
#include "logging.h"

const char* const DOOR_SENSOR_ENTITY_IDS[] = {
    "binary_sensor.door_backyard_contact",
    "binary_sensor.door_0_coffee_contact",
    "binary_sensor.door_0_kitchen_contact"
};
const size_t DOOR_SENSOR_COUNT = sizeof(DOOR_SENSOR_ENTITY_IDS) / sizeof(DOOR_SENSOR_ENTITY_IDS[0]);

void subscribeToState(const char* entityId) {
    JsonDocument subscription;
    subscription["id"] = hass_message_id++;
    subscription["type"] = "subscribe_trigger";
    JsonObject trigger = subscription.createNestedObject("trigger");
    trigger["platform"] = "state";
    trigger["entity_id"] = entityId;

    String serializedSubscription;
    serializeJson(subscription, serializedSubscription);
    client.send(serializedSubscription);
    logInfo("Subscribing to " + String(entityId));
}

bool isDoorSensor(const char* entityId) {
    for (size_t i = 0; i < DOOR_SENSOR_COUNT; i++) {
        if (strcmp(entityId, DOOR_SENSOR_ENTITY_IDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool isTrueState(const char* state) {
    return strcmp(state, "on") == 0 || strcmp(state, "true") == 0;
}

// This will be called from the main setup()
void setupHass() {
    static bool callbacksRegistered = false;
    if (!callbacksRegistered) {
        client.onMessage(onMessage);
        client.onEvent(onEvent);
        callbacksRegistered = true;
    }

    client.connect(HASS_HOST, HASS_PORT, "/api/websocket");
}

// This will be called from the main loop()
void loopHass() {
    client.poll();
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
      logInfo("Response: " + payload);

      // Parse JSON to get the state value
      // Using DynamicJsonDocument to avoid potential overflow with larger HA responses.
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        logError("Failed to parse Home Assistant response: " + String(error.c_str()));
        http.end();
        return "error";
      }
      const char* state = doc["state"] | "error";
      logInfo("Sensor state: " + String(state));
      http.end();
      return state;
    } else {
      logError("Error on HTTP request: " + String(httpResponseCode));
      http.end();
      return "error";
    }
}

void onMessage(WebsocketsMessage message) {
    logInfo("Got Message: " + message.data());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) {
        logError("deserializeJson() failed: " + String(error.c_str()));
        return;
    }

    const char* type = doc["type"] | "";

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

        subscribeToState(SENSOR_1_ENTITY_ID);
        subscribeToState(SENSOR_2_ENTITY_ID);
        subscribeToState(SENSOR_3_ENTITY_ID);
        for (size_t i = 0; i < DOOR_SENSOR_COUNT; i++) {
            subscribeToState(DOOR_SENSOR_ENTITY_IDS[i]);
        }
        refreshHassStates = true;

    } else if (strcmp(type, "auth_invalid") == 0) {
        logError("Auth invalid. Check your HASS_TOKEN.");
    } else if (strcmp(type, "event") == 0) {
        JsonObject event = doc["event"];
        JsonObject variables = event["variables"];
        JsonObject trigger = variables["trigger"];
        const char* entity_id = trigger["entity_id"] | "";
        const char* state = trigger["to_state"]["state"] | "unknown";
        const char* previousState = trigger["from_state"]["state"] | "";

        if (isDoorSensor(entity_id)) {
            if (isTrueState(state) && !isTrueState(previousState)) {
                logInfo("Door opened: " + String(entity_id));
                playSound(1, 50, 100);
            }
        } else if (strcmp(entity_id, SENSOR_1_ENTITY_ID) == 0) {
            copySensorState(sensor_1_state, state);
            logInfo("Sensor 1 state updated: " + String(sensor_1_state));
            playSound();
        } else if (strcmp(entity_id, SENSOR_2_ENTITY_ID) == 0) {
            copySensorState(sensor_2_state, state);
            logInfo("Sensor 2 state updated: " + String(sensor_2_state));
            Serial.println(sensor_2_state);
            playSound();
        } else if (strcmp(entity_id, SENSOR_3_ENTITY_ID) == 0) {
            copySensorState(sensor_3_state, state);
            logInfo("Sensor 3 state updated: " + String(sensor_3_state));
            Serial.println(sensor_3_state);

            String stateLower = String(sensor_3_state);
            stateLower.toLowerCase();

            if (stateLower == "on") {
                playSound(3, 50, 100);
            } else {
                playSound(1, 50, 300);
            }
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
