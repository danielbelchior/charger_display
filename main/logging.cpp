#include "logging.h"
#include "declarations.h"

// Redefine log methods to use datetime
void logInfo(String message){_logMessage("INFO", message);}
void logWarning(String message){_logMessage("WARNING", message);}
void logError(String message){_logMessage("ERROR", message);}
void logDebug(String message){_logMessage("DEBUG", message);}
void _logMessage(String level, String message) {
    // Sanitize message (avoid breaking JSON)
    message.replace("\"", "' ");
    message.replace("\n", " ");

    String currentDateTime = timeClient.getFormattedTime();
    message = currentDateTime + " " + level + ": " + message;

    // Get board IP
    //String ipAddress = WiFi.localIP().toString();

    // if (WiFi.status() == WL_CONNECTED) {
    //     udp.beginPacket(LOG_UDP_IP, LOG_UDP_PORT);
    //     udp.write((const uint8_t*)message.c_str(), message.length());
    //     udp.endPacket();
    // }

    Serial.println(message);

    // log history buffer
    // Shift all existing messages down by one position
    for (int i = LOG_BUFFER_SIZE - 1; i > 0; i--) {
        strcpy(logBuffer[i], logBuffer[i - 1]);
    }

    // Add the new message at the beginning (index 0)
    strncpy(logBuffer[0], message.c_str(), 99);
    logBuffer[0][99] = '\0';
}
