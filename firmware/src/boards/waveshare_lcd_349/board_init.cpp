#include "board.h"
#include "io_expander.h"
#include <Arduino.h>
#include <Wire.h>

// Bring up both I2C buses, then the IO expander (power hold, backlight rail,
// panel reset) before anything touches the display. main.cpp calls this
// first, then display_hal_init().

extern "C" void board_init(void) {
    Wire.begin(IIC_SDA, IIC_SCL, IIC_HZ);     // system bus
    Wire1.begin(TP_SDA, TP_SCL, TP_HZ);       // touch bus
    io_expander_init();
    Serial.printf("%s: buses up (sys %d/%d, touch %d/%d)\n",
                  BOARD_NAME, IIC_SDA, IIC_SCL, TP_SDA, TP_SCL);
}
