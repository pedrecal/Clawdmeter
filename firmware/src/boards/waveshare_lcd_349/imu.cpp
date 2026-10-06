#include "../../hal/imu_hal.h"
#include "board.h"
#include <Arduino.h>
#include <Wire.h>
#include <SensorQMI8658.hpp>

// QMI8658 is populated on the system bus. Initialized for bus health and
// future auto-rotation; this port runs at a fixed landscape orientation.

static SensorQMI8658 imu;

void imu_hal_init(void) {
    if (!imu.begin(Wire, QMI8658_ADDR, IIC_SDA, IIC_SCL)) {
        Serial.println("QMI8658 init failed");
        return;
    }
    Serial.println("QMI8658 init OK (rotation disabled on this board)");
}

void imu_hal_tick(void) {
    // No-op — fixed orientation.
}

uint8_t imu_hal_rotation_quadrant(void) { return 0; }
