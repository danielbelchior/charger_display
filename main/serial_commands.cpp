#include "serial_commands.h"
#include "logging.h"
#include "display.h"

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
