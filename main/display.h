#pragma once

#include "config.h"
#include "display_frame.h"

void displaySetup();
void displaySetBrightness(uint8_t brightness);
void displaySetOtaInProgress(bool inProgress);

// Scrolls `text` across the matrix DISPLAY_MESSAGE_PASSES times, then returns
// to the normal layout. `text` must stay valid until then (use a literal).
void displayShowMessage(const char* text, Color color);

// Call every loop(): advances the animation once per DISPLAY_FRAME_MS and the
// message scroll once per DISPLAY_MESSAGE_STEP_MS.
void displayLoop();

// Advance the animation by one frame and redraw.
void displayTick();
// Redraw the current state without advancing the animation.
void displayRender();

// 0xRRGGBB of a pixel as currently shown.
uint32_t displayPixelColor(int row, int col);
