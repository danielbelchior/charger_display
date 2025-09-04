#ifndef DISPLAY_H
#define DISPLAY_H

#include "declarations.h"

// --- Function Prototypes ---

// Setup
void setupDisplay();

// Core Drawing
void render_matrix();
void drawMatrix();
void clean_display();

// Drawing Primitives & Animations
void getNextBorderPoint();
void updateItem(int row, int col, Color color);
void drawBorder();
void noWifi(int row);

// State-specific Patterns
void state_unknown(int bike, int row);
void state_charging(int bike, int row);
void state_disconnected(int bike, int row);

// Color & Pixel Utilities
int getPixelIndex(int row, int col);
uint8_t highlightBrightness(uint8_t tone);
uint32_t highlightPixel(int row, int col, uint32_t rgbColor);
Color stringToColor(String colorName);
void createArray8x8();


#endif // DISPLAY_H
