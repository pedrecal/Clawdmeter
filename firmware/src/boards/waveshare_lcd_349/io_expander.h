#pragma once
#include <stdint.h>

// TCA9554 8-bit I2C IO expander on the system bus (Wire). Board-private.
//
// Must be initialized BEFORE the display: LCD reset (EXIO5) and the backlight
// boost enable (EXIO1) live here. It also raises SYS_EN (EXIO6), the battery
// power-hold line — without it the board drops out when running on battery.

bool io_expander_init(void);
void io_expander_set(uint8_t pin, bool high);
bool io_expander_get(uint8_t pin);   // reads the INPUT register
