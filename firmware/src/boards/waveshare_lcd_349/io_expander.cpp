#include "io_expander.h"
#include "board.h"
#include <Arduino.h>
#include <Wire.h>

// TCA9554 register map
#define IOX_REG_INPUT    0x00
#define IOX_REG_OUTPUT   0x01
#define IOX_REG_POLARITY 0x02
#define IOX_REG_CONFIG   0x03   // 1 = input, 0 = output

// Inputs: EXIO0 (TP_INT), EXIO2/3 (IMU), EXIO4 (RTC). Outputs: EXIO1 (BL_EN),
// EXIO5 (LCD_RST), EXIO6 (SYS_EN), EXIO7 (PA_EN).
#define IOX_CONFIG_MASK  ((1u << EXIO_TP_INT) | (1u << EXIO_IMU_INT1) | \
                          (1u << EXIO_IMU_INT2) | (1u << EXIO_RTC_INT))

static uint8_t output_state = 0x00;

static bool write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool read_reg(uint8_t reg, uint8_t& val) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)TCA9554_ADDR, (uint8_t)1) != 1) return false;
    val = Wire.read();
    return true;
}

bool io_expander_init(void) {
    if (!write_reg(IOX_REG_CONFIG, IOX_CONFIG_MASK)) {
        Serial.println("TCA9554 init failed (config)");
        return false;
    }
    // Power hold up and backlight boost on immediately; panel held in reset,
    // amp off.
    output_state = (1u << EXIO_SYS_EN) | (1u << EXIO_BL_EN);
    write_reg(IOX_REG_OUTPUT, output_state);
    delay(20);
    // Release the panel reset. The AXS15231B wants ~120 ms before commands.
    output_state |= (1u << EXIO_LCD_RST);
    write_reg(IOX_REG_OUTPUT, output_state);
    delay(120);
    Serial.println("TCA9554 init OK");
    return true;
}

void io_expander_set(uint8_t pin, bool high) {
    if (pin > 7) return;
    if (high) output_state |= (1u << pin);
    else      output_state &= ~(1u << pin);
    write_reg(IOX_REG_OUTPUT, output_state);
}

bool io_expander_get(uint8_t pin) {
    if (pin > 7) return false;
    uint8_t in = 0;
    if (!read_reg(IOX_REG_INPUT, in)) return false;
    return (in >> pin) & 1u;
}
