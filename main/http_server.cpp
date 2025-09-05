#include "http_server.h"
#include "logging.h"
#include "display.h"

void setupHttpServer() {
    server.begin();
    logInfo("HTTP server started on port 80");
}

void handleHttpRequests() {
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    // logInfo("New HTTP client.");
    
    // Wait for data to be available
    unsigned long timeout = millis() + 200; // 200ms timeout
    while(!client.available() && millis() < timeout) {
        delay(1);
    }
    if(!client.available()){
        logWarning("HTTP client connected but sent no data.");
        client.stop();
        return;
    }

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush(); // Clear the rest of the request from the buffer

    // Match the request
    if (req.indexOf("GET /status") != -1) {
        // Send HTTP headers
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();

        // Create JSON response
        DynamicJsonDocument jsonDoc(4096);

        jsonDoc["uptime"] = millis() / 1000;
        jsonDoc["ip_address"] = WiFi.localIP().toString();
        jsonDoc["wifi_connected"] = wifi_connected;
        jsonDoc["ws_connected"] = ws_connected;
        jsonDoc["sensor_1_state"] = sensor_1_state;
        jsonDoc["sensor_2_state"] = sensor_2_state;
        jsonDoc["displayBrightness"] = displayBrightness;
        jsonDoc["should_render"] = should_render;
        jsonDoc["free_heap"] = ESP.getFreeHeap();

        JsonArray logData = jsonDoc.createNestedArray("logBuffer");
        for (int i = 0; i < LOG_BUFFER_SIZE; i++) {
            if (logBuffer[i] != "") {
                logData.add(logBuffer[i]);
            }
        }

        JsonArray displayData = jsonDoc.createNestedArray("displayArray");
        for (int i = 0; i < MATRIX_HEIGHT; i++) {
            JsonArray row = displayData.createNestedArray();
            for (int j = 0; j < MATRIX_WIDTH; j++) {
                Color colorValue = (Color)displayArray[i][j];
                uint32_t rgbColor = translateColor(colorValue);
                rgbColor = highlightPixel(i, j, rgbColor);
                row.add(rgbColor);
            }
        }

        String response;
        serializeJson(jsonDoc, response);
        client.print(response);

    } else if (req.indexOf("GET /beep") != -1) {
        logInfo("HTTP GET /beep request received.");
        playSound();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();
        client.print("{\"status\":\"ok\", \"action\":\"beep\"}");

    } else if (req.indexOf("GET /config/display_brightness/") != -1) {
    req.replace(" HTTP/1.1", "");
    String valueStr = req.substring(req.lastIndexOf('/') + 1);
        int spaceIndex = valueStr.indexOf(' ');
        if (spaceIndex != -1) {
            valueStr = valueStr.substring(0, spaceIndex);
        }
        logDebug(req.substring(0, 100));
        logDebug(req.substring(req.lastIndexOf('/') + 1));
        
        int newBrightness = valueStr.toInt();
        logInfo(String("HTTP GET /config/display_brightness/") + newBrightness + " request received.");
        logInfo(String("Updating brightness from:") + displayBrightness + " to: " + newBrightness);

        if (newBrightness >= 0 && newBrightness <= 255) {
            displayBrightness = newBrightness;
            strip.setBrightness(displayBrightness);
            logInfo("Display brightness set to: " + String(newBrightness));

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.print("{\"status\":\"ok\", \"variable\":\"displayBrightness\", \"new_value\":\"" + String(newBrightness) + "\"}");
        } else {
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.print("{\"status\":\"error\", \"message\":\"Brightness value must be between 0 and 255.\"}");
        }

    } else if (req.indexOf("GET /config/update_state/") != -1) {
    req.replace(" HTTP/1.1", "");
    String temp = req.substring(req.indexOf("updatestate/") + 12);
        int firstSlash = temp.indexOf('/');
        
        if (firstSlash != -1) {
            String idStr = temp.substring(0, firstSlash);
            String stateStr = temp.substring(firstSlash + 1);
            int spaceIndex = stateStr.indexOf(' ');
            if (spaceIndex != -1) {
                stateStr = stateStr.substring(0, spaceIndex);
            }

            int sensorId = idStr.toInt();
            logInfo(String("HTTP GET /config/update_state/") + sensorId + "/" + stateStr + " request received.");

            if (sensorId == 1) {
                sensor_1_state = stateStr;
                logInfo("Manually updated sensor 1 state to: " + stateStr);
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println("Access-Control-Allow-Origin: *");
                client.println("Connection: close");
                client.println();
                client.print("{\"status\":\"ok\", \"sensor_id\":1, \"new_state\":\"" + stateStr + "\"}");
            } else if (sensorId == 2) {
                sensor_2_state = stateStr;
                logInfo("Manually updated sensor 2 state to: " + stateStr);
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println("Access-Control-Allow-Origin: *");
                client.println("Connection: close");
                client.println();
                client.print("{\"status\":\"ok\", \"sensor_id\":2, \"new_state\":\"" + stateStr + "\"}");
            } else {
                client.println("HTTP/1.1 400 Bad Request");
                client.println("Content-Type: application/json");
                client.println("Access-Control-Allow-Origin: *");
                client.println("Connection: close");
                client.println();
                client.print("{\"status\":\"error\", \"message\":\"Invalid sensor_id. Must be 1 or 2.\"}");
            }
        } else {
            logWarning("HTTP/1.1 400 Bad Request");
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: application/json");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.print("{\"status\":\"error\", \"message\":\"Malformed URL. Use /config/updatestate/<id>/<value>\"}");
        }

    } else {
        logWarning("HTTP/1.1 404 Not Found");
        // Send 404 Not Found
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/html");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();
        client.println("<!DOCTYPE html><html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>");
    }

    delay(1); // Give the client time to receive the data
    client.stop();
    // logInfo("HTTP client disconnected.");
}
