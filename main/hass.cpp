#include "hass.h"
#include "logging.h"

// This will be called from the main setup()
void setupHass() {
    // Register callbacks and connect
    client.onMessage(onMessage);
    client.onEvent(onEvent);
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
      deserializeJson(doc, payload);
      const char* state = doc["state"];
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
            strcpy(sensor_1_state, state);
            logInfo("Sensor 1 state updated: " + String(sensor_1_state));
            playSound();
        } else if (strcmp(entity_id, SENSOR_2_ENTITY_ID) == 0) {
            strcpy(sensor_2_state, state);
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
