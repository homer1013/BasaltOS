#pragma once

#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_RMT_CAPTURE_MAX_EDGES 64u

typedef struct {
    int start_level;
    uint32_t edges;
    uint32_t levels[HAL_RMT_CAPTURE_MAX_EDGES + 1u];
    uint32_t durations_us[HAL_RMT_CAPTURE_MAX_EDGES + 1u];
} hal_rmt_capture_t;

int hal_rmt_init(hal_rmt_t *rmt,
                 int tx_pin,
                 int rx_pin,
                 uint32_t resolution_hz,
                 int enable_tx,
                 int enable_rx);

int hal_rmt_deinit(hal_rmt_t *rmt);

int hal_rmt_get_ready(hal_rmt_t *rmt, int *tx_ready_out, int *rx_ready_out);

int hal_rmt_pulse(hal_rmt_t *rmt, uint32_t on_us, uint32_t off_us, uint32_t count);

int hal_rmt_capture(hal_rmt_t *rmt,
                    uint32_t window_ms,
                    uint32_t poll_us,
                    hal_rmt_capture_t *out);

int hal_rmt_loopback(hal_rmt_t *rmt,
                     uint32_t on_us,
                     uint32_t off_us,
                     uint32_t count,
                     uint32_t window_ms,
                     uint32_t poll_us,
                     hal_rmt_capture_t *out);

#ifdef __cplusplus
}
#endif
