#include "../../hal/touch_hal.h"
#include "board.h"
#include <Arduino.h>
#include <Wire.h>

// AXS15231B in-cell touch on its own I2C bus (Wire1). Reader vendored from
// the rsvpnano port (MIT): an 11-byte command, then an 8-byte packet:
//   [0]      0x00 on a valid frame (idle frames may be non-zero filled)
//   [1]      point count (1..4)
//   [2..3]   long-axis coordinate (12 bit)  — along the 640 px side
//   [4..5]   short-axis coordinate (12 bit) — along the 172 px side
// Coordinates are produced in portrait panel space, then mapped to the
// landscape LVGL space with the inverse of display.cpp's rotation.
//
// Polled every TP_POLL_MS rather than interrupt-driven: TP_INT is routed to
// the IO expander (EXIO0), and a 19-byte I2C exchange at 300 kHz is well
// under the 5 ms budget.

static const uint8_t READ_CMD[] = {0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};

static uint32_t last_poll_ms = 0;
static bool     cur_pressed  = false;
static uint16_t cur_x = 0, cur_y = 0;
static uint8_t  empty_polls  = 0;

// A frame without a valid finger: release only after TP_RELEASE_DEBOUNCE of
// them in a row, so a single glitch frame does not split one tap in two.
static void note_empty(void) {
    if (cur_pressed && ++empty_polls < TP_RELEASE_DEBOUNCE) return;
    cur_pressed = false;
    empty_polls = 0;
}

static bool read_packet(uint8_t* buf) {
    Wire1.beginTransmission(TP_ADDR);
    Wire1.write(READ_CMD, sizeof(READ_CMD));
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom((uint8_t)TP_ADDR, (uint8_t)8) != 8) return false;
    for (int i = 0; i < 8; i++) buf[i] = Wire1.read();
    return true;
}

static void poll(void) {
    uint8_t d[8];
    if (!read_packet(d)) { note_empty(); return; }
    if (d[0] != 0) { note_empty(); return; }
    uint8_t points = d[1];
    if (points == 0 || points > 4) { note_empty(); return; }
    empty_polls = 0;

    uint16_t raw_long  = ((d[2] & 0x0F) << 8) | d[3];
    uint16_t raw_short = ((d[4] & 0x0F) << 8) | d[5];

    // Portrait panel coordinates (same convention as rsvpnano: Y flipped).
    uint16_t px = raw_short < PANEL_WIDTH ? raw_short : PANEL_WIDTH - 1;
    uint16_t py = raw_long >= PANEL_HEIGHT ? 0 : (PANEL_HEIGHT - 1 - raw_long);

    // Inverse of display.cpp's rotation.
    uint16_t lx, ly;
#if LCD_ROTATION == 1
    lx = py;
    ly = PANEL_WIDTH - 1 - px;
#else
    lx = PANEL_HEIGHT - 1 - py;
    ly = px;
#endif
#if TP_MIRROR_X
    lx = LCD_WIDTH - 1 - lx;
#endif
#if TP_MIRROR_Y
    ly = LCD_HEIGHT - 1 - ly;
#endif
#if TP_DEBUG
    if (!cur_pressed) Serial.printf("touch %u %u (raw long=%u short=%u)\n", lx, ly, raw_long, raw_short);
#endif
    cur_x = lx; cur_y = ly; cur_pressed = true;
}

void touch_hal_init(void) {
    Wire1.beginTransmission(TP_ADDR);
    if (Wire1.endTransmission() == 0)
        Serial.printf("Touch AXS15231B found at 0x%02X\n", TP_ADDR);
    else
        Serial.printf("Touch AXS15231B NOT found at 0x%02X\n", TP_ADDR);
}

void touch_hal_read(uint16_t* x, uint16_t* y, bool* pressed) {
    uint32_t now = millis();
    if (now - last_poll_ms >= TP_POLL_MS) {
        last_poll_ms = now;
        poll();
    }
    *x = cur_x;
    *y = cur_y;
    *pressed = cur_pressed;
}
