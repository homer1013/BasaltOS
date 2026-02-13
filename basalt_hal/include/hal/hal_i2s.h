#pragma once

#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_I2S_DIAG_MAX_EDGES 64u

typedef struct {
    int start_level;
    uint32_t edges;
    uint32_t levels[HAL_I2S_DIAG_MAX_EDGES + 1u];
    uint32_t durations_us[HAL_I2S_DIAG_MAX_EDGES + 1u];
} hal_i2s_diag_capture_t;

int hal_i2s_diag_init(hal_i2s_t *i2s,
                      int bclk_pin,
                      int ws_pin,
                      int dout_pin,
                      int din_pin,
                      int sample_rate_hz,
                      int bits_per_sample);
int hal_i2s_diag_deinit(hal_i2s_t *i2s);
int hal_i2s_diag_get_ready(hal_i2s_t *i2s, int *tx_ready_out, int *rx_ready_out);
int hal_i2s_diag_get_stats(hal_i2s_t *i2s, int *sample_rate_hz_out, int *bits_per_sample_out);

int hal_i2s_diag_loopback(hal_i2s_t *i2s,
                          uint32_t on_us,
                          uint32_t off_us,
                          uint32_t count,
                          uint32_t window_ms,
                          uint32_t poll_us,
                          hal_i2s_diag_capture_t *out);

#ifdef __cplusplus
}
#endif
