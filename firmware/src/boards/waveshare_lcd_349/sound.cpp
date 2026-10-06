#include "../../hal/sound_hal.h"
#include "board.h"

#if BOARD_HAS_SOUND

#include <Arduino.h>
#include "../../chime.h"
#include "io_expander.h"

// ES8311 codec + NS4150 amp, same shared chime engine as the other boards.
// The codec sits on the system bus (Wire = I2C port 0, which the engine
// uses). The amp enable is TCA9554 EXIO7, not a GPIO. Mic path (ES7210) is
// not used by the chime.

static void amp_enable(bool on) {
    io_expander_set(EXIO_PA_EN, on);
}

void sound_hal_init(void) {
    const ChimeConfig cfg = {
        SND_I2S_MCLK, SND_I2S_BCLK, SND_I2S_WS, SND_I2S_DOUT, SND_I2S_DIN,
        SND_SAMPLE_RATE, SND_ES8311_ADDR, 65, amp_enable
    };
    chime_init(cfg);
}

void sound_hal_play_reset(void) { chime_play(); }
void sound_hal_tick(void)       { chime_tick(); }

#endif  // BOARD_HAS_SOUND
