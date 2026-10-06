#include "../../hal/power_hal.h"
#include "../../hal/display_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>
#include <esp_sleep.h>

// No I2C PMU on this board. Battery percentage comes from the VBAT divider on
// BAT_ADC_PIN; charging / VBUS state is unknowable, so both report false.
//
// The PWR-role button is SYS_OUT (GPIO 16, active LOW). Same software edge
// synthesis as the LCD-1.54 port:
//   short    — fired on release if the hold was shorter than PWR_LONG_MS
//   long     — fired once when a hold crosses PWR_LONG_MS
//   release  — fired on every release edge
//
// Power OFF at 8 s mirrors the AXP boards: drop SYS_EN on the expander (cuts
// the rail on battery) and deep-sleep (pseudo-off on USB, BOOT wakes).

#define BATTERY_POLL_MS  2000
#define PWR_POLL_MS      50
#define PWR_LONG_MS      1500
#define PWR_OFF_HOLD_MS  8000

static int      cached_pct        = -1;
static bool     pwr_pressed_flag  = false;
static bool     pwr_long_flag     = false;
static bool     pwr_released_flag = false;
static bool     last_pwr_state    = false;
static uint32_t pwr_press_started_ms = 0;
static bool     pwr_long_fired    = false;
static uint32_t last_battery_ms   = 0;
static uint32_t last_pwr_ms       = 0;

static void sample_battery(void) {
    uint32_t mv = 0;
    for (int i = 0; i < 4; i++) mv += analogReadMilliVolts(BAT_ADC_PIN);
    float vbat = (mv / 4) * BAT_VOLT_DIVIDER / 1000.0f;
    if (vbat < 3.0f) { cached_pct = -1; return; }   // no battery connected
    int pct = (int)((vbat - 3.3f) * (100.0f / 0.9f) + 0.5f);
    cached_pct = pct < 0 ? 0 : pct > 100 ? 100 : pct;
}

static void power_off(void) {
    Serial.println("PWR held 8s — powering off");
    Serial.flush();
    display_hal_set_brightness(0);
    delay(50);
    io_expander_set(EXIO_SYS_EN, false);   // rail off on battery
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);   // BOOT wakes on USB
    delay(200);
    esp_deep_sleep_start();
}

void power_hal_init(void) {
    pinMode(BTN_PWR_GPIO, INPUT_PULLUP);
    analogReadResolution(12);
    // First analogRead configures the channel; only then can the attenuation
    // be set without the core logging "not configured as analog channel".
    (void)analogRead(BAT_ADC_PIN);
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
    sample_battery();
}

void power_hal_tick(void) {
    uint32_t now = millis();

    if (now - last_battery_ms >= BATTERY_POLL_MS) {
        last_battery_ms = now;
        sample_battery();
    }
    if (now - last_pwr_ms >= PWR_POLL_MS) {
        last_pwr_ms = now;
        bool pwr_now = (digitalRead(BTN_PWR_GPIO) == LOW);
        if (pwr_now && !last_pwr_state) {
            pwr_press_started_ms = now;
            pwr_long_fired = false;
        } else if (pwr_now && last_pwr_state) {
            if (!pwr_long_fired && (now - pwr_press_started_ms >= PWR_LONG_MS)) {
                pwr_long_flag  = true;
                pwr_long_fired = true;
            }
            if (now - pwr_press_started_ms >= PWR_OFF_HOLD_MS) power_off();
        } else if (!pwr_now && last_pwr_state) {
            pwr_released_flag = true;
            if (!pwr_long_fired) pwr_pressed_flag = true;
        }
        last_pwr_state = pwr_now;
    }
}

int  power_hal_battery_pct(void) { return cached_pct; }
bool power_hal_is_charging(void) { return false; }
bool power_hal_is_vbus_in(void)  { return false; }

bool power_hal_pwr_pressed(void) {
    if (pwr_pressed_flag) { pwr_pressed_flag = false; return true; }
    return false;
}
bool power_hal_pwr_long_pressed(void) {
    if (pwr_long_flag) { pwr_long_flag = false; return true; }
    return false;
}
bool power_hal_pwr_released(void) {
    if (pwr_released_flag) { pwr_released_flag = false; return true; }
    return false;
}
