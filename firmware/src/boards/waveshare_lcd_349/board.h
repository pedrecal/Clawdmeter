#pragma once

// Waveshare ESP32-S3-Touch-LCD-3.49 **V2** (PCB silkscreen "Rev1.1", case
// label "V2"; the "B" suffix only changes the battery to a LiPo pack).
// 172×640 AXS15231B IPS strip over QSPI, AXS15231B in-cell capacitive touch on
// its own I2C bus, TCA9554 IO expander gating LCD reset / backlight boost /
// power hold / amp enable, QMI8658 IMU, PCF85063 RTC, ES8311 + ES7210 audio,
// 16 MB flash, 8 MB OPI PSRAM, battery via ADC divider (no I2C PMU).
//
// The panel is portrait-native (172 wide × 640 tall). We run it as a
// 640×172 LANDSCAPE strip: LVGL renders landscape and display.cpp rotates
// each flushed strip on the CPU before pushing it to the panel. Touch
// coordinates get the inverse mapping in touch.cpp.
//
// Pin map: Waveshare V2 factory firmware (ESP-IDF 11_FactoryProgram
// user_config.h) cross-checked with the rsvpnano `rev2` Arduino port, both
// hardware-tested on V2. V1 boards differ (backlight on GPIO 8, EXIO_INT on
// GPIO 42, LCD reset on GPIO 21) and are NOT supported by this folder.

#define BOARD_NAME           "Waveshare LCD 3.49"

// ---- Display geometry ----
// What LVGL and the UI see (post-rotation, landscape).
#define LCD_WIDTH            640
#define LCD_HEIGHT           172
// Native panel geometry (portrait) — used only inside display.cpp / touch.cpp.
#define PANEL_WIDTH          172
#define PANEL_HEIGHT         640
// Quarter turns clockwise from portrait to our landscape. 1 or 3 — pick the
// one that puts the USB-C connector where you want it. Touch follows.
#define LCD_ROTATION         1

// ---- QSPI display pins ----
#define LCD_CS               9
#define LCD_SCLK             10
#define LCD_SDIO0            11
#define LCD_SDIO1            12
#define LCD_SDIO2            13
#define LCD_SDIO3            14
#define LCD_TE               21    // tearing-effect output, unused
#define LCD_RESET            GFX_NOT_DEFINED   // reset is TCA9554 EXIO5
#define LCD_QSPI_HZ          40000000

// ---- Backlight ----
// Brightness is LEDC PWM on GPIO 42 (the AXS15231B has no usable brightness
// command here). The AP3032 boost rail must also be enabled via EXIO1.
// The PWM curve has a large dead zone at low duty — rsvpnano maps its 1–100 %
// scale onto duty 102..255 so "dim" is dim rather than off. Polarity was
// observed active-LOW in the rsvpnano Arduino port; flip BL_ACTIVE_LOW if the
// panel comes up dark at full brightness.
#define LCD_BL               42
#define BL_PWM_HZ            25000
#define BL_ACTIVE_LOW        1
#define BL_MIN_DUTY          102

// ---- I2C buses ----
// System bus: TCA9554, QMI8658, PCF85063, ES8311, ES7210. This is `Wire` so
// the shared chime engine and SensorLib find the codec/IMU without changes.
#define IIC_SDA              47
#define IIC_SCL              48
#define IIC_HZ               400000
// Touch bus: the AXS15231B touch controller sits alone on a second bus.
#define TP_SDA               17
#define TP_SCL               18
#define TP_HZ                300000

// ---- Touch (AXS15231B in-cell, vendored reader in touch.cpp) ----
#define TP_ADDR              0x3B
#define TP_POLL_MS           30     // rsvpnano polls at 30 ms; INT is on EXIO0
// Mirroring verified on hardware 2026-10-06 (five-point tap test): the
// rotation mapping alone is correct, no mirroring needed.
#define TP_MIRROR_X          0
#define TP_MIRROR_Y          0
// Print "touch x y" on every press edge over USB serial (calibration aid).
#define TP_DEBUG             0
// The controller occasionally returns one empty frame mid-press; hold the
// pressed state across this many empty polls before reporting a release.
#define TP_RELEASE_DEBOUNCE  2

// ---- TCA9554 IO expander (system bus) ----
#define TCA9554_ADDR         0x20
#define EXIO_TP_INT          0     // input, active LOW
#define EXIO_BL_EN           1     // output, HIGH = backlight boost on
#define EXIO_IMU_INT1        2     // input
#define EXIO_IMU_INT2        3     // input
#define EXIO_RTC_INT         4     // input
#define EXIO_LCD_RST         5     // output, LOW = panel in reset
#define EXIO_SYS_EN          6     // output, HIGH = hold the battery rail up
#define EXIO_PA_EN           7     // output, HIGH = speaker amp on
#define EXIO_INT_GPIO        8     // expander interrupt line (unused, polled)

// ---- Other chips on the system bus ----
#define QMI8658_ADDR         0x6B
#define PCF85063_ADDR        0x51

// ---- Battery (ADC divider, no I2C PMU) ----
// VBAT → 3.0× divider → GPIO 4. The ETA6098 charger exposes nothing readable.
#define BAT_ADC_PIN          4
#define BAT_VOLT_DIVIDER     3.0f

// ---- Buttons (active-LOW) ----
#define BTN_BACK_GPIO        0     // BOOT — primary, Space (PTT)
#define BTN_PWR_GPIO         16    // SYS_OUT from the power-key circuit — PWR role

// ---- Audio (ES8311 codec + NS4150 amp; ES7210 mics unused) ----
#define SND_I2S_MCLK         7
#define SND_I2S_BCLK         15
#define SND_I2S_WS           46
#define SND_I2S_DOUT         45    // ESP → ES8311 (speaker)
#define SND_I2S_DIN          6     // ES7210 → ESP, unused by the chime
#define SND_SAMPLE_RATE      44100 // must match the embedded PCM (bell_pcm.h)
#define SND_ES8311_ADDR      0x18
// Amp enable is EXIO7 on the expander, not a GPIO — see sound.cpp.

// ---- Capability flags ----
#define BOARD_HAS_SECONDARY_BUTTON 0
#define BOARD_HAS_ROTATION         0    // fixed landscape; IMU rotation later
#define BOARD_HAS_IMU              1
#define BOARD_HAS_BATTERY          1
#define BOARD_HAS_IO_EXPANDER      1
#define BOARD_HAS_SOUND            1
