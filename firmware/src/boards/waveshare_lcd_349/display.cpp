#include "../../hal/display_hal.h"
#include "board.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <string.h>

// AXS15231B over QSPI, portrait-native 172×640, driven as a 640×172 landscape
// strip.
//
// Two panel quirks shape this file (both learned from the rsvpnano port,
// which is hardware-tested on this exact board):
//
//  1. The panel does not reliably accept arbitrary CASET/RASET windows. The
//     only robust write is "native rows 0..N from the top". So instead of
//     pushing each LVGL strip to its own window, we keep a full portrait
//     framebuffer in PSRAM, rotate every LVGL flush into it, and push rows
//     0..dirty_end in one transfer from display_hal_tick().
//  2. Brightness is LEDC PWM on LCD_BL with a large dead zone at low duty
//     (and, per rsvpnano, active-LOW polarity). See backlight_write().
//
// Rotation (LCD_ROTATION 1): landscape (x, y) -> panel (PANEL_WIDTH-1-y, x).
// So landscape column x lands on panel row x; a flush touching landscape
// columns up to x2 dirties panel rows 0..x2.

static Arduino_DataBus*   bus = nullptr;
static Arduino_AXS15231B* gfx = nullptr;

static uint16_t* fb = nullptr;          // PANEL_WIDTH × PANEL_HEIGHT, portrait
static int32_t   dirty_rows = 0;        // panel rows 0..dirty_rows-1 need a push
static bool      fb_ready = false;

void display_hal_init(void) {
    bus = new Arduino_ESP32QSPI(
        LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
    // (bus, rst, rotation, ips, w, h, col_off1, row_off1, col_off2, row_off2)
    gfx = new Arduino_AXS15231B(
        bus, LCD_RESET, 0 /* rotation done in software */, false,
        PANEL_WIDTH, PANEL_HEIGHT, 0, 0, 0, 0);
}

static void backlight_write(uint8_t level) {
    uint16_t duty = level == 0 ? 0
                  : BL_MIN_DUTY + ((uint16_t)level * (255 - BL_MIN_DUTY)) / 255;
#if BL_ACTIVE_LOW
    duty = 255 - duty;
#endif
    ledcWrite(LCD_BL, duty);
}

static void push_rows(int32_t rows) {
    if (!gfx || !fb || rows <= 0) return;
    if (rows > PANEL_HEIGHT) rows = PANEL_HEIGHT;
    gfx->draw16bitRGBBitmap(0, 0, fb, PANEL_WIDTH, rows);
}

void display_hal_begin(void) {
    gfx->begin(LCD_QSPI_HZ);
    fb = (uint16_t*)heap_caps_aligned_alloc(16, (size_t)PANEL_WIDTH * PANEL_HEIGHT * 2,
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!fb) {
        Serial.println("display: framebuffer alloc failed");
    } else {
        memset(fb, 0, (size_t)PANEL_WIDTH * PANEL_HEIGHT * 2);
        fb_ready = true;
        push_rows(PANEL_HEIGHT);
    }
    ledcAttach(LCD_BL, BL_PWM_HZ, 8 /* bits */);
    backlight_write(200);
}

void display_hal_set_brightness(uint8_t level) {
    backlight_write(level);
}

void display_hal_fill_screen(uint16_t color) {
    if (!fb_ready) return;
    for (int32_t i = 0; i < PANEL_WIDTH * PANEL_HEIGHT; i++) fb[i] = color;
    dirty_rows = 0;
    push_rows(PANEL_HEIGHT);
}

// Rotate a landscape strip into the portrait framebuffer. Pixels arrive in
// LVGL's native byte order for RGB565 and the panel expects the same, so no
// swap here (matches the other QSPI ports).
void display_hal_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h,
                             const uint16_t* pixels) {
    if (!fb_ready) return;
#if LCD_ROTATION == 1
    // (lx, ly) -> (px = PANEL_WIDTH-1-ly, py = lx)
    for (int32_t ly = 0; ly < h; ly++) {
        const uint16_t* src = pixels + ly * w;
        int32_t px = PANEL_WIDTH - 1 - (y + ly);
        if (px < 0 || px >= PANEL_WIDTH) continue;
        for (int32_t lx = 0; lx < w; lx++) {
            int32_t py = x + lx;
            if (py < 0 || py >= PANEL_HEIGHT) continue;
            fb[py * PANEL_WIDTH + px] = src[lx];
        }
    }
    int32_t end = x + w;                 // panel rows 0..end-1 touched
#else
    // (lx, ly) -> (px = ly, py = PANEL_HEIGHT-1-lx)
    for (int32_t ly = 0; ly < h; ly++) {
        const uint16_t* src = pixels + ly * w;
        int32_t px = y + ly;
        if (px < 0 || px >= PANEL_WIDTH) continue;
        for (int32_t lx = 0; lx < w; lx++) {
            int32_t py = PANEL_HEIGHT - 1 - (x + lx);
            if (py < 0 || py >= PANEL_HEIGHT) continue;
            fb[py * PANEL_WIDTH + px] = src[lx];
        }
    }
    int32_t end = PANEL_HEIGHT;          // touched rows reach the bottom
#endif
    if (end > dirty_rows) dirty_rows = end;
}

// Push the dirty prefix of the framebuffer once per main-loop iteration,
// after LVGL has flushed every strip of the current refresh.
void display_hal_tick(void) {
    if (dirty_rows > 0) {
        push_rows(dirty_rows);
        dirty_rows = 0;
    }
}

void display_hal_round_area(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2) {
    *x1 = *x1 & ~1;
    *y1 = *y1 & ~1;
    *x2 = *x2 | 1;
    *y2 = *y2 | 1;
}
