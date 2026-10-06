#include "../../hal/input_hal.h"
#include "board.h"
#include <Arduino.h>

// Only BOOT as a direct input. The PWR-role button (GPIO 16) is owned by
// power.cpp so the hold-to-pair / power-off gestures stay board-agnostic.

void input_hal_init(void) {
    pinMode(BTN_BACK_GPIO, INPUT_PULLUP);
}

bool input_hal_is_held(InputButton btn) {
    switch (btn) {
    case INPUT_BTN_PRIMARY:
        return digitalRead(BTN_BACK_GPIO) == LOW;
    case INPUT_BTN_SECONDARY:
        return false;   // not present on this board
    }
    return false;
}
