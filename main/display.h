#pragma once

#include "config.h"

enum Color : uint8_t { BLACK = 0, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA };

void displaySetup();
void displaySetBrightness(uint8_t brightness);
void displaySetOtaInProgress(bool inProgress);

// Advance the animation by one frame and redraw.
void displayTick();
// Redraw the current state without advancing the animation.
void displayRender();

// 0xRRGGBB of a pixel as currently shown, including the moving highlight.
uint32_t displayPixelColor(int row, int col);
