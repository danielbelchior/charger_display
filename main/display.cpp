#include <Adafruit_NeoPixel.h>

#include "display.h"
#include "app_state.h"

static_assert(FRAME_ROWS == MATRIX_HEIGHT && FRAME_COLS == MATRIX_WIDTH, "display_frame.h assumes an 8x8 matrix");

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static Frame frame;
static uint32_t animationStep = 0;
static uint32_t lastFrameAt = 0;
static bool otaInProgress = false;

// Scrolling message overlay (displayShowMessage).
static const char* messageText = nullptr;
static Color messageColor = WHITE;
static uint32_t messageStartedAt = 0;
static uint32_t messageShownStep = UINT32_MAX;

static LinkState currentLink() {
    if (!app.wifiConnected) {
        return LinkState::WifiDown;
    }
    return app.hassConnected ? LinkState::Ok : LinkState::HassDown;
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

// Message step for `now`, or UINT32_MAX once all passes have scrolled by.
static uint32_t messageStep(uint32_t now) {
    const uint32_t step = (now - messageStartedAt) / DISPLAY_MESSAGE_STEP_MS;
    return step < messageScrollSteps(messageText) * DISPLAY_MESSAGE_PASSES ? step : UINT32_MAX;
}

static void renderMessage(uint32_t step) {
    messageShownStep = step;
    composeMessage(messageText, messageColor, step % messageScrollSteps(messageText), frame);
    pushFrame();
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

void displayShowMessage(const char* text, Color color) {
    messageText = text;
    messageColor = color;
    messageStartedAt = millis();
    messageShownStep = UINT32_MAX;
    displayRender();
}

void displayLoop() {
    const uint32_t now = millis();
    if (now - lastFrameAt >= DISPLAY_FRAME_MS) {
        lastFrameAt = now;
        displayTick();
    }
    if (messageText && !otaInProgress) {
        const uint32_t step = messageStep(now);
        if (step == UINT32_MAX) {
            messageText = nullptr;
            displayRender();
        } else if (step != messageShownStep) {
            renderMessage(step);
        }
    }
}

void displayTick() {
    // Frozen while WiFi or Home Assistant is down: the batteries keep their
    // last frame and the status column shows the fault.
    if (currentLink() == LinkState::Ok) {
        animationStep = (animationStep + 1) % FRAME_CYCLE_STEPS;
    }
    displayRender();
}

void displayRender() {
    if (messageText && !otaInProgress) {
        const uint32_t step = messageStep(millis());
        if (step != UINT32_MAX) {
            renderMessage(step);
            return;
        }
        messageText = nullptr;
    }
    FrameInputs in;
    in.charger[0] = chargerStateFromText(app.sensorState[0]);
    in.charger[1] = chargerStateFromText(app.sensorState[1]);
    in.link = currentLink();
    in.otaInProgress = otaInProgress;
    in.step = animationStep;
    composeFrame(in, frame);
    pushFrame();
}

uint32_t displayPixelColor(int row, int col) {
    return colorRgb(static_cast<Color>(frame[row][col]));
}
