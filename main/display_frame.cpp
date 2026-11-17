#include "display_frame.h"

#include <string.h>

namespace {

constexpr int CHARGER_COL[2] = {0, 5};
constexpr Color TERMINAL_COLOR[2] = {CYAN, MAGENTA};
// Charger 2 runs ahead so the two halves never look mirrored.
constexpr uint32_t CHARGER_STEP_OFFSET[2] = {0, 3};
constexpr int STATUS_COLS[2] = {3, 4};
constexpr int BODY_TOP = 1;
constexpr int BODY_BOTTOM = 7;
constexpr int BODY_WIDTH = 3;

void setPixel(Frame f, int row, int col, Color color) {
    if (row >= 0 && row < FRAME_ROWS && col >= 0 && col < FRAME_COLS) {
        f[row][col] = color;
    }
}

void drawOutline(Frame f, int x0, Color color) {
    for (int row = BODY_TOP; row <= BODY_BOTTOM; row++) {
        for (int c = 0; c < BODY_WIDTH; c++) {
            if (row == BODY_TOP || row == BODY_BOTTOM || c != 1) {
                setPixel(f, row, x0 + c, color);
            }
        }
    }
}

void drawCharger(Frame f, int index, ChargerState state, uint32_t step) {
    const int x0 = CHARGER_COL[index];
    setPixel(f, 0, x0 + 1, TERMINAL_COLOR[index]);

    switch (state) {
        case ChargerState::Charging: {
            drawOutline(f, x0, BLUE);
            const uint32_t level = (step + CHARGER_STEP_OFFSET[index]) % FRAME_CYCLE_STEPS;
            for (uint32_t i = 0; i < level; i++) {
                for (int c = 0; c < BODY_WIDTH; c++) {
                    setPixel(f, BODY_BOTTOM - static_cast<int>(i), x0 + c, GREEN);
                }
            }
            break;
        }
        case ChargerState::Disconnected:
            drawOutline(f, x0, BLUE);
            break;
        case ChargerState::Unknown:
            drawOutline(f, x0, YELLOW);
            if (step % 2 == 0) {
                setPixel(f, 4, x0 + 1, YELLOW);
            }
            break;
    }
}

void drawStatusColumn(Frame f, LinkState link, uint32_t step) {
    for (int row = 0; row < FRAME_ROWS; row++) {
        Color color = BLACK;
        switch (link) {
            case LinkState::Ok:
                color = row == static_cast<int>(step % FRAME_ROWS) ? WHITE : BLACK;
                break;
            case LinkState::HassDown:
                color = row % 2 == 0 ? YELLOW : BLACK;
                break;
            case LinkState::WifiDown:
                color = RED;
                break;
        }
        for (int col : STATUS_COLS) {
            setPixel(f, row, col, color);
        }
    }
}

void drawOta(Frame f) {
    for (int row = 0; row < FRAME_ROWS; row++) {
        for (int col = 0; col < FRAME_COLS; col++) {
            if ((row + col) % 2 == 0) {
                setPixel(f, row, col, YELLOW);
            }
        }
    }
}

}  // namespace

ChargerState chargerStateFromText(const char* state) {
    if (state && strcmp(state, "charging") == 0) {
        return ChargerState::Charging;
    }
    if (state && strcmp(state, "disconnected") == 0) {
        return ChargerState::Disconnected;
    }
    return ChargerState::Unknown;
}

void composeFrame(const FrameInputs& in, Frame out) {
    memset(out, BLACK, sizeof(Frame));
    if (in.otaInProgress) {
        drawOta(out);
        return;
    }
    drawCharger(out, 0, in.charger[0], in.step);
    drawCharger(out, 1, in.charger[1], in.step);
    drawStatusColumn(out, in.link, in.step);
}

uint32_t colorRgb(Color color) {
    switch (color) {
        case WHITE:
            return 0xFFFFFF;
        case RED:
            return 0xFF0000;
        case GREEN:
            return 0x00FF00;
        case BLUE:
            return 0x0000FF;
        case YELLOW:
            return 0xFFFF00;
        case CYAN:
            return 0x00FFFF;
        case MAGENTA:
            return 0xFF00FF;
        case BLACK:
        default:
            return 0;
    }
}

namespace {

constexpr int GLYPH_ROWS = 5;

struct Glyph {
    char ch;
    uint8_t width;
    const char* rows[GLYPH_ROWS];
};

// clang-format off
constexpr Glyph GLYPHS[] = {
    {'O', 3, {"###",
              "#.#",
              "#.#",
              "#.#",
              "###"}},
    {'N', 4, {"#..#",
              "##.#",
              "#.##",
              "#..#",
              "#..#"}},
    {'F', 3, {"###",
              "#..",
              "##.",
              "#..",
              "#.."}},
};
// clang-format on

constexpr uint8_t BLANK_WIDTH = 3;

const Glyph* findGlyph(char ch) {
    for (const Glyph& g : GLYPHS) {
        if (g.ch == ch) {
            return &g;
        }
    }
    return nullptr;
}

int glyphWidth(char ch) {
    const Glyph* g = findGlyph(ch);
    return g ? g->width : BLANK_WIDTH;
}

}  // namespace

int messageWidth(const char* text) {
    if (!text || !*text) {
        return 0;
    }
    int width = 0;
    for (const char* p = text; *p; p++) {
        width += glyphWidth(*p) + 1;
    }
    return width - 1;
}

uint32_t messageScrollSteps(const char* text) {
    return static_cast<uint32_t>(FRAME_COLS + messageWidth(text) - 1);
}

void composeMessage(const char* text, Color color, uint32_t step, Frame out) {
    memset(out, BLACK, sizeof(Frame));
    if (!text) {
        return;
    }
    int x = FRAME_COLS - 1 - static_cast<int>(step);
    for (const char* p = text; *p; p++) {
        const Glyph* g = findGlyph(*p);
        const int width = glyphWidth(*p);
        if (g) {
            for (int r = 0; r < GLYPH_ROWS; r++) {
                for (int c = 0; c < width; c++) {
                    if (g->rows[r][c] == '#') {
                        setPixel(out, MESSAGE_TOP_ROW + r, x + c, color);
                    }
                }
            }
        }
        x += width + 1;
    }
}
