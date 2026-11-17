#pragma once

// Builds one 8x8 frame from the display state. Arduino-free so every pattern
// can be unit tested on the host (tests/host/test_display_frame.cpp).
//
// Layout: columns 0-2 are charger 1, columns 5-7 charger 2, each a battery
// with a one-pixel terminal on row 0 and a 3x7 body on rows 1-7. Columns 3-4
// are the status column (heartbeat, or the WiFi / Home Assistant fault).
//
// Every pixel is off or one full-intensity palette color: the hardware only
// dims the whole display, so nothing here relies on per-LED brightness.

#include <stdint.h>

enum Color : uint8_t { BLACK = 0, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA };

constexpr int FRAME_ROWS = 8;
constexpr int FRAME_COLS = 8;
// Charging fill levels 0..7 rows, so one animation cycle is 8 steps.
constexpr uint32_t FRAME_CYCLE_STEPS = 8;

enum class ChargerState : uint8_t { Charging, Disconnected, Unknown };
enum class LinkState : uint8_t { Ok, HassDown, WifiDown };

struct FrameInputs {
    ChargerState charger[2] = {ChargerState::Unknown, ChargerState::Unknown};
    LinkState link = LinkState::Ok;
    bool otaInProgress = false;
    // Animation step. The caller advances it only while the link is Ok, so a
    // connection fault freezes the batteries on their last known frame.
    uint32_t step = 0;
};

using Frame = uint8_t[FRAME_ROWS][FRAME_COLS];

// "charging" and "disconnected" map to their states; anything else is Unknown.
ChargerState chargerStateFromText(const char* state);

void composeFrame(const FrameInputs& in, Frame out);

// 0xRRGGBB of a palette color.
uint32_t colorRgb(Color color);

// --- Scrolling text -----------------------------------------------------------
// Short words in a 5-row font, scrolled right to left across rows 1-5. Only the
// glyphs below exist; any other character renders as a blank of glyph width.

constexpr int MESSAGE_TOP_ROW = 1;

// Width in columns, including one blank column between glyphs.
int messageWidth(const char* text);

// Steps of one pass: step 0 shows the first column at the right edge, the last
// step shows the last column at the left edge.
uint32_t messageScrollSteps(const char* text);

void composeMessage(const char* text, Color color, uint32_t step, Frame out);
