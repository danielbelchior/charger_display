#include "display.h"
#include "logging.h"

// This function will be called from the main setup()
void setupDisplay() {
    strip.begin();
    strip.setBrightness(displayBrightness);
    strip.show(); // Initialize all pixels to 'off'
}

void createArray8x8() {
    for (int i = 0; i < MATRIX_HEIGHT; i++) {
        for (int j = 0; j < MATRIX_WIDTH; j++) {
            displayArray[i][j] = BLACK;
        }
    }
}

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
            rgbColor = highlightPixel(i, j, rgbColor);
            strip.setPixelColor(pixelIndex, rgbColor);
        }
    }
    strip.show();
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
