#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "hal/hal_i2s.h"

typedef struct {
    int bclk_pin;
    int ws_pin;
    int dout_pin;
    int din_pin;
    int sample_rate_hz;
    int bits_per_sample;
    int initialized;
} hal_i2s_impl_t;

_Static_assert(sizeof(hal_i2s_impl_t) <= sizeof(((hal_i2s_t *)0)->_opaque),
               "hal_i2s_t opaque storage too small for hal_i2s_impl_t");

static inline hal_i2s_impl_t *I(hal_i2s_t *i2s) {
    return (hal_i2s_impl_t *)i2s->_opaque;
}

int hal_i2s_diag_init(hal_i2s_t *i2s,
                      int bclk_pin,
                      int ws_pin,
                      int dout_pin,
                      int din_pin,
                      int sample_rate_hz,
                      int bits_per_sample) {
    if (!i2s || bclk_pin < 0 || ws_pin < 0) return -EINVAL;
    if (sample_rate_hz <= 0 || bits_per_sample <= 0) return -EINVAL;
    hal_i2s_impl_t *impl = I(i2s);
    impl->bclk_pin = bclk_pin;
    impl->ws_pin = ws_pin;
    impl->dout_pin = dout_pin;
    impl->din_pin = din_pin;
    impl->sample_rate_hz = sample_rate_hz;
    impl->bits_per_sample = bits_per_sample;
    impl->initialized = 1;
    return 0;
}

int hal_i2s_diag_deinit(hal_i2s_t *i2s) {
    if (!i2s) return -EINVAL;
    hal_i2s_impl_t *impl = I(i2s);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_i2s_diag_get_ready(hal_i2s_t *i2s, int *tx_ready_out, int *rx_ready_out) {
    if (!i2s || !tx_ready_out || !rx_ready_out) return -EINVAL;
    hal_i2s_impl_t *impl = I(i2s);
    if (!impl->initialized) return -EINVAL;
    *tx_ready_out = (impl->dout_pin >= 0) ? 1 : 0;
    *rx_ready_out = (impl->din_pin >= 0) ? 1 : 0;
    return 0;
}

int hal_i2s_diag_get_stats(hal_i2s_t *i2s, int *sample_rate_hz_out, int *bits_per_sample_out) {
    if (!i2s || !sample_rate_hz_out || !bits_per_sample_out) return -EINVAL;
    hal_i2s_impl_t *impl = I(i2s);
    if (!impl->initialized) return -EINVAL;
    *sample_rate_hz_out = impl->sample_rate_hz;
    *bits_per_sample_out = impl->bits_per_sample;
    return 0;
}

int hal_i2s_diag_loopback(hal_i2s_t *i2s,
                          uint32_t on_us,
                          uint32_t off_us,
                          uint32_t count,
                          uint32_t window_ms,
                          uint32_t poll_us,
                          hal_i2s_diag_capture_t *out) {
    if (!i2s || !out || on_us == 0 || off_us == 0 || count == 0 || window_ms == 0 || poll_us == 0) {
        return -EINVAL;
    }
    hal_i2s_impl_t *impl = I(i2s);
    if (!impl->initialized) return -EINVAL;
    if (impl->dout_pin < 0 || impl->din_pin < 0) return -ENOSYS;
    memset(out, 0, sizeof(*out));
    out->start_level = 0;
    out->edges = (count * 2u > HAL_I2S_DIAG_MAX_EDGES) ? HAL_I2S_DIAG_MAX_EDGES : count * 2u;
    for (uint32_t i = 0; i <= out->edges; ++i) {
        out->levels[i] = (i % 2u == 0u) ? 0u : 1u;
        out->durations_us[i] = (i % 2u == 0u) ? off_us : on_us;
    }
    return 0;
}
