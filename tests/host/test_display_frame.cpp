#include <initializer_list>

#include "../../main/display_frame.h"
#include "test.h"

static void compose(FrameInputs in, Frame out) {
    composeFrame(in, out);
}

static FrameInputs inputs(ChargerState a, ChargerState b, LinkState link = LinkState::Ok, uint32_t step = 0) {
    FrameInputs in;
    in.charger[0] = a;
    in.charger[1] = b;
    in.link = link;
    in.step = step;
    return in;
}

static bool isOutline(int row, int c) {
    return row >= 1 && row <= 7 && (row == 1 || row == 7 || c != 1);
}

// Number of body rows (from the bottom) that are fully green for the charger at x0.
static int greenRows(const Frame f, int x0) {
    int rows = 0;
    for (int row = 7; row >= 1; row--) {
        if (f[row][x0] != GREEN || f[row][x0 + 1] != GREEN || f[row][x0 + 2] != GREEN) {
            break;
        }
        rows++;
    }
    return rows;
}

TEST_CASE(charger_state_from_text) {
    CHECK(chargerStateFromText("charging") == ChargerState::Charging);
    CHECK(chargerStateFromText("disconnected") == ChargerState::Disconnected);
    CHECK(chargerStateFromText("unknown") == ChargerState::Unknown);
    CHECK(chargerStateFromText("unavailable") == ChargerState::Unknown);
    CHECK(chargerStateFromText("Charging") == ChargerState::Unknown);
    CHECK(chargerStateFromText("") == ChargerState::Unknown);
    CHECK(chargerStateFromText(nullptr) == ChargerState::Unknown);
}

TEST_CASE(terminals_identify_the_chargers) {
    Frame f;
    compose(inputs(ChargerState::Disconnected, ChargerState::Disconnected), f);
    CHECK_EQ(f[0][1], CYAN);
    CHECK_EQ(f[0][6], MAGENTA);
    for (int col : {0, 2, 5, 7}) {
        CHECK_EQ(f[0][col], BLACK);
    }
}

TEST_CASE(disconnected_is_an_empty_blue_battery) {
    Frame f;
    compose(inputs(ChargerState::Disconnected, ChargerState::Disconnected), f);
    for (int x0 : {0, 5}) {
        for (int row = 1; row <= 7; row++) {
            for (int c = 0; c < 3; c++) {
                CHECK_EQ(f[row][x0 + c], isOutline(row, c) ? BLUE : BLACK);
            }
        }
    }
}

TEST_CASE(charging_fills_one_row_per_step_and_restarts) {
    Frame f;
    for (uint32_t step = 0; step < 16; step++) {
        compose(inputs(ChargerState::Charging, ChargerState::Charging, LinkState::Ok, step), f);
        CHECK_EQ(greenRows(f, 0), static_cast<int>(step % 8));
        CHECK_EQ(greenRows(f, 5), static_cast<int>((step + 3) % 8));
    }
}

TEST_CASE(charging_rows_above_the_fill_show_the_outline) {
    Frame f;
    compose(inputs(ChargerState::Charging, ChargerState::Disconnected, LinkState::Ok, 4), f);
    for (int row = 1; row <= 3; row++) {
        for (int c = 0; c < 3; c++) {
            CHECK_EQ(f[row][c], isOutline(row, c) ? BLUE : BLACK);
        }
    }
    for (int row = 4; row <= 7; row++) {
        for (int c = 0; c < 3; c++) {
            CHECK_EQ(f[row][c], GREEN);
        }
    }
}

TEST_CASE(unknown_is_yellow_with_blinking_center) {
    Frame on;
    Frame off;
    compose(inputs(ChargerState::Unknown, ChargerState::Unknown, LinkState::Ok, 2), on);
    compose(inputs(ChargerState::Unknown, ChargerState::Unknown, LinkState::Ok, 3), off);
    for (int x0 : {0, 5}) {
        CHECK_EQ(on[4][x0 + 1], YELLOW);
        CHECK_EQ(off[4][x0 + 1], BLACK);
        for (int row = 1; row <= 7; row++) {
            for (int c = 0; c < 3; c++) {
                if (isOutline(row, c)) {
                    CHECK_EQ(on[row][x0 + c], YELLOW);
                    CHECK_EQ(off[row][x0 + c], YELLOW);
                }
            }
        }
    }
}

TEST_CASE(heartbeat_walks_down_the_status_column) {
    Frame f;
    for (uint32_t step = 0; step < 8; step++) {
        compose(inputs(ChargerState::Charging, ChargerState::Unknown, LinkState::Ok, step), f);
        for (int row = 0; row < 8; row++) {
            const uint8_t want = row == static_cast<int>(step) ? WHITE : BLACK;
            CHECK_EQ(f[row][3], want);
            CHECK_EQ(f[row][4], want);
        }
    }
}

TEST_CASE(hass_down_shows_dotted_yellow_status_column) {
    Frame f;
    compose(inputs(ChargerState::Charging, ChargerState::Disconnected, LinkState::HassDown, 5), f);
    for (int row = 0; row < 8; row++) {
        const uint8_t want = row % 2 == 0 ? YELLOW : BLACK;
        CHECK_EQ(f[row][3], want);
        CHECK_EQ(f[row][4], want);
    }
    // Batteries still show the last known state.
    CHECK_EQ(greenRows(f, 0), 5);
    CHECK_EQ(f[7][5], BLUE);
}

TEST_CASE(wifi_down_shows_solid_red_status_column) {
    Frame f;
    compose(inputs(ChargerState::Disconnected, ChargerState::Charging, LinkState::WifiDown, 2), f);
    for (int row = 0; row < 8; row++) {
        CHECK_EQ(f[row][3], RED);
        CHECK_EQ(f[row][4], RED);
    }
    CHECK_EQ(greenRows(f, 5), 5);
}

TEST_CASE(ota_checkerboard_replaces_everything) {
    FrameInputs in = inputs(ChargerState::Charging, ChargerState::Charging, LinkState::WifiDown, 3);
    in.otaInProgress = true;
    Frame f;
    compose(in, f);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            CHECK_EQ(f[row][col], (row + col) % 2 == 0 ? YELLOW : BLACK);
        }
    }
}

TEST_CASE(every_pixel_is_a_full_palette_color) {
    const ChargerState states[] = {ChargerState::Charging, ChargerState::Disconnected, ChargerState::Unknown};
    const LinkState links[] = {LinkState::Ok, LinkState::HassDown, LinkState::WifiDown};
    Frame f;
    for (ChargerState a : states) {
        for (ChargerState b : states) {
            for (LinkState link : links) {
                for (uint32_t step = 0; step < 2 * FRAME_CYCLE_STEPS; step++) {
                    compose(inputs(a, b, link, step), f);
                    for (int row = 0; row < 8; row++) {
                        for (int col = 0; col < 8; col++) {
                            CHECK(f[row][col] <= MAGENTA);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE(large_steps_wrap_cleanly) {
    Frame a;
    Frame b;
    compose(inputs(ChargerState::Charging, ChargerState::Unknown, LinkState::Ok, 0xFFFFFFF8u), a);
    compose(inputs(ChargerState::Charging, ChargerState::Unknown, LinkState::Ok, 0), b);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            CHECK_EQ(a[row][col], b[row][col]);
        }
    }
}

TEST_CASE(palette_rgb_values) {
    CHECK_EQ(colorRgb(BLACK), 0u);
    CHECK_EQ(colorRgb(WHITE), 0xFFFFFFu);
    CHECK_EQ(colorRgb(RED), 0xFF0000u);
    CHECK_EQ(colorRgb(GREEN), 0x00FF00u);
    CHECK_EQ(colorRgb(BLUE), 0x0000FFu);
    CHECK_EQ(colorRgb(YELLOW), 0xFFFF00u);
    CHECK_EQ(colorRgb(CYAN), 0x00FFFFu);
    CHECK_EQ(colorRgb(MAGENTA), 0xFF00FFu);
}

// --- Scrolling text ---------------------------------------------------------

static bool frameEmpty(const Frame f) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (f[row][col] != BLACK) {
                return false;
            }
        }
    }
    return true;
}

TEST_CASE(message_widths_and_steps) {
    CHECK_EQ(messageWidth("ON"), 8);
    CHECK_EQ(messageWidth("OFF"), 11);
    CHECK_EQ(messageWidth(""), 0);
    CHECK_EQ(messageWidth(nullptr), 0);
    CHECK_EQ(messageWidth("O?"), 7);  // unknown glyphs are blanks of width 3
    CHECK_EQ(messageScrollSteps("ON"), 15u);
    CHECK_EQ(messageScrollSteps("OFF"), 18u);
}

TEST_CASE(message_on_fully_visible_mid_scroll) {
    Frame f;
    compose(inputs(ChargerState::Charging, ChargerState::Charging), f);  // dirty buffer first
    composeMessage("ON", GREEN, 7, f);
    const char* want[8] = {
        "........", "###.#..#", "#.#.##.#", "#.#.#.##", "#.#.#..#", "###.#..#", "........", "........",
    };
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            CHECK_EQ(f[row][col], want[row][col] == '#' ? GREEN : BLACK);
        }
    }
}

TEST_CASE(message_enters_at_right_edge_and_leaves_at_left_edge) {
    Frame f;
    composeMessage("OFF", RED, 0, f);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            const bool lit = col == 7 && row >= MESSAGE_TOP_ROW && row < MESSAGE_TOP_ROW + 5;
            CHECK_EQ(f[row][col], lit ? RED : BLACK);
        }
    }
    // Last step: the right column of the last F, lit only on its top row, sits at col 0.
    composeMessage("OFF", RED, messageScrollSteps("OFF") - 1, f);
    CHECK_EQ(f[1][0], RED);
    for (int row = 2; row < 8; row++) {
        CHECK_EQ(f[row][0], BLACK);
    }
    for (int row = 0; row < 8; row++) {
        for (int col = 1; col < 8; col++) {
            CHECK_EQ(f[row][col], BLACK);
        }
    }
    composeMessage("OFF", RED, messageScrollSteps("OFF"), f);
    CHECK(frameEmpty(f));
}

TEST_CASE(message_uses_only_rows_1_to_5_and_one_color) {
    Frame f;
    for (uint32_t step = 0; step < messageScrollSteps("OFF"); step++) {
        composeMessage("OFF", RED, step, f);
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (row < 1 || row > 5) {
                    CHECK_EQ(f[row][col], BLACK);
                } else {
                    CHECK(f[row][col] == BLACK || f[row][col] == RED);
                }
            }
        }
    }
}

TEST_CASE(message_null_text_is_blank) {
    Frame f;
    composeMessage(nullptr, GREEN, 3, f);
    CHECK(frameEmpty(f));
}
