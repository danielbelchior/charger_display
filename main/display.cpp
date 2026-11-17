#include <Adafruit_NeoPixel.h>

#include "display.h"
#include "app_state.h"

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static uint8_t frame[MATRIX_HEIGHT][MATRIX_WIDTH];

// Animation phase, cycles 1..6 (the inner rows of the matrix).
static int animationRow = 1;
// A single brighter pixel walks clockwise around the border once per 28 frames.
static int highlightIndex = 0;
static int highlightX = 0;
static int highlightY = 0;
static bool otaInProgress = false;

static constexpr uint8_t HIGHLIGHT_BOOST = 206;

static void clearFrame() {
    memset(frame, BLACK, sizeof(frame));
}

static void setPixel(int row, int col, Color color) {
    if (row >= 0 && row < MATRIX_HEIGHT && col >= 0 && col < MATRIX_WIDTH) {
        frame[row][col] = color;
    }
}

static void advanceHighlight() {
    const int size = MATRIX_WIDTH;
    const int borderPoints = (size * 4) - 4;
    const int i = highlightIndex;

    if (i < size) {
        highlightX = i;
        highlightY = 0;
    } else if (i < 2 * size - 1) {
        highlightX = size - 1;
        highlightY = i - (size - 1);
    } else if (i < 3 * size - 2) {
        highlightX = (size - 1) - (i - (2 * size - 2));
        highlightY = size - 1;
    } else {
        highlightX = 0;
        highlightY = (size - 1) - (i - (3 * size - 3));
    }

    highlightIndex = (highlightIndex + 1) % borderPoints;
}

// Charger 1 uses columns 0-3, charger 2 uses columns 4-7.
static int chargerColumn(int charger) {
    return charger == 1 ? 0 : 4;
}

static void drawBorder() {
    for (int col : {1, 2}) {
        setPixel(0, col, CYAN);
        setPixel(7, col, CYAN);
    }
    for (int col : {5, 6}) {
        setPixel(0, col, MAGENTA);
        setPixel(7, col, MAGENTA);
    }
    for (int row = 0; row < MATRIX_HEIGHT; row++) {
        setPixel(row, 0, CYAN);
        setPixel(row, 3, CYAN);
        setPixel(row, 4, MAGENTA);
        setPixel(row, 7, MAGENTA);
    }
}

static void drawUnknown(int charger) {
    const int col = chargerColumn(charger);
    const int row = (animationRow % 2 == 0) ? 2 : 3;
    setPixel(row, col + 1, RED);
    setPixel(row, col + 2, RED);
    setPixel(row + 2, col + 1, RED);
    setPixel(row + 2, col + 2, RED);
}

static void drawCharging(int charger) {
    const int col = chargerColumn(charger);
    setPixel(animationRow, col + 1, BLUE);
    setPixel(animationRow, col + 2, BLUE);
}

static void drawDisconnected(int charger) {
    const int col = chargerColumn(charger);
    for (int row = 1; row < 7; row++) {
        setPixel(row, col + (((row + animationRow) % 2 == 0) ? 1 : 2), YELLOW);
    }
}

static void drawCharger(int charger, const char* state) {
    if (strcmp(state, "charging") == 0) {
        drawCharging(charger);
    } else if (strcmp(state, "disconnected") == 0) {
        drawDisconnected(charger);
    } else if (strcmp(state, "unknown") == 0) {
        drawUnknown(charger);
    }
}

// Blinking corner pixel plus a staircase pattern; colors tell WiFi vs HA apart.
static void drawConnectionProblem(Color blinkA, Color blinkB, Color stairs) {
    setPixel(0, 0, (animationRow % 2 == 0) ? blinkA : blinkB);
    for (int r = 1; r < 8; r += 2) {
        for (int c = r; c < 8; c++) {
            setPixel(c, 7 - r, stairs);
        }
    }
}

static void drawOta() {
    for (int row = 0; row < MATRIX_HEIGHT; row++) {
        for (int col = 0; col < MATRIX_WIDTH; col++) {
            if ((row + col) % 2 == 0) {
                setPixel(row, col, YELLOW);
            }
        }
    }
}

static uint32_t colorToRgb(Color color) {
    switch (color) {
        case WHITE:
            return Adafruit_NeoPixel::Color(255, 255, 255);
        case RED:
            return Adafruit_NeoPixel::Color(255, 0, 0);
        case GREEN:
            return Adafruit_NeoPixel::Color(0, 255, 0);
        case BLUE:
            return Adafruit_NeoPixel::Color(0, 0, 255);
        case YELLOW:
            return Adafruit_NeoPixel::Color(255, 255, 0);
        case CYAN:
            return Adafruit_NeoPixel::Color(0, 255, 255);
        case MAGENTA:
            return Adafruit_NeoPixel::Color(255, 0, 255);
        case BLACK:
        default:
            return 0;
    }
}

static uint8_t boost(uint8_t tone) {
    const unsigned value = tone + HIGHLIGHT_BOOST;
    return value > 255 ? 255 : value;
}

static void composeFrame() {
    clearFrame();
    if (otaInProgress) {
        drawOta();
    } else if (app.wifiConnected && app.hassConnected) {
        drawBorder();
        drawCharger(1, app.sensorState[0]);
        drawCharger(2, app.sensorState[1]);
    } else if (app.wifiConnected) {
        drawConnectionProblem(GREEN, YELLOW, BLUE);
    } else {
        drawConnectionProblem(BLUE, RED, RED);
    }
}

static void pushFrame() {
    for (int row = 0; row < MATRIX_HEIGHT; row++) {
        for (int col = 0; col < MATRIX_WIDTH; col++) {
            // The matrix is wired row by row, left to right (not serpentine).
            strip.setPixelColor(row * MATRIX_WIDTH + col, displayPixelColor(row, col));
        }
    }
    strip.show();
}

void displaySetup() {
    strip.begin();
    strip.setBrightness(app.displayBrightness);
    strip.show();
}

void displaySetBrightness(uint8_t brightness) {
    app.displayBrightness = brightness;
    strip.setBrightness(brightness);
    pushFrame();
}

void displaySetOtaInProgress(bool inProgress) {
    otaInProgress = inProgress;
    displayRender();
}

void displayTick() {
    animationRow = animationRow >= 6 ? 1 : animationRow + 1;
    advanceHighlight();
    displayRender();
}

void displayRender() {
    composeFrame();
    pushFrame();
}

uint32_t displayPixelColor(int row, int col) {
    const uint32_t rgb = colorToRgb(static_cast<Color>(frame[row][col]));
    if (row != highlightY || col != highlightX) {
        return rgb;
    }
    return Adafruit_NeoPixel::Color(boost(rgb >> 16), boost(rgb >> 8), boost(rgb));
}
