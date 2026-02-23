#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "hal/hal_rmt.h"

typedef struct {
    int tx_pin;
    int rx_pin;
    uint32_t resolution_hz;
    int tx_enabled;
    int rx_enabled;
    int initialized;
} hal_rmt_impl_t;

_Static_assert(sizeof(hal_rmt_impl_t) <= sizeof(((hal_rmt_t *)0)->_opaque),
               "hal_rmt_t opaque storage too small for hal_rmt_impl_t");

static inline hal_rmt_impl_t *R(hal_rmt_t *rmt) {
    return (hal_rmt_impl_t *)rmt->_opaque;
}

int hal_rmt_init(hal_rmt_t *rmt,
                 int tx_pin,
                 int rx_pin,
                 uint32_t resolution_hz,
                 int enable_tx,
                 int enable_rx) {
    if (!rmt || resolution_hz == 0) return -EINVAL;
    if (enable_tx && tx_pin < 0) return -EINVAL;
    if (enable_rx && rx_pin < 0) return -EINVAL;
    hal_rmt_impl_t *impl = R(rmt);
    impl->tx_pin = tx_pin;
    impl->rx_pin = rx_pin;
    impl->resolution_hz = resolution_hz;
    impl->tx_enabled = (enable_tx != 0) ? 1 : 0;
    impl->rx_enabled = (enable_rx != 0) ? 1 : 0;
    impl->initialized = 1;
    return 0;
}

int hal_rmt_deinit(hal_rmt_t *rmt) {
    if (!rmt) return -EINVAL;
    hal_rmt_impl_t *impl = R(rmt);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_rmt_get_ready(hal_rmt_t *rmt, int *tx_ready_out, int *rx_ready_out) {
    if (!rmt || !tx_ready_out || !rx_ready_out) return -EINVAL;
    hal_rmt_impl_t *impl = R(rmt);
    if (!impl->initialized) return -EINVAL;
    *tx_ready_out = impl->tx_enabled;
    *rx_ready_out = impl->rx_enabled;
    return 0;
}

int hal_rmt_pulse(hal_rmt_t *rmt, uint32_t on_us, uint32_t off_us, uint32_t count) {
    if (!rmt || on_us == 0 || off_us == 0 || count == 0) return -EINVAL;
    hal_rmt_impl_t *impl = R(rmt);
    if (!impl->initialized) return -EINVAL;
    if (!impl->tx_enabled) return -ENOSYS;
    return 0;
}

int hal_rmt_capture(hal_rmt_t *rmt,
                    uint32_t window_ms,
                    uint32_t poll_us,
                    hal_rmt_capture_t *out) {
    if (!rmt || !out || window_ms == 0 || poll_us == 0) return -EINVAL;
    hal_rmt_impl_t *impl = R(rmt);
    if (!impl->initialized) return -EINVAL;
    if (!impl->rx_enabled) return -ENOSYS;
    memset(out, 0, sizeof(*out));
    out->start_level = 0;
    out->edges = 2;
    out->levels[0] = 0;
    out->levels[1] = 1;
    out->levels[2] = 0;
    out->durations_us[0] = poll_us;
    out->durations_us[1] = poll_us;
    out->durations_us[2] = (uint32_t)window_ms * 1000u;
    return 0;
}

int hal_rmt_loopback(hal_rmt_t *rmt,
                     uint32_t on_us,
                     uint32_t off_us,
                     uint32_t count,
                     uint32_t window_ms,
                     uint32_t poll_us,
                     hal_rmt_capture_t *out) {
    if (!rmt || !out || on_us == 0 || off_us == 0 || count == 0 || window_ms == 0 || poll_us == 0) {
        return -EINVAL;
    }
    hal_rmt_impl_t *impl = R(rmt);
    if (!impl->initialized) return -EINVAL;
    if (!impl->tx_enabled || !impl->rx_enabled) return -ENOSYS;
    memset(out, 0, sizeof(*out));
    out->start_level = 0;
    out->edges = (count * 2u > HAL_RMT_CAPTURE_MAX_EDGES) ? HAL_RMT_CAPTURE_MAX_EDGES : count * 2u;
    for (uint32_t i = 0; i <= out->edges; ++i) {
        out->levels[i] = (i % 2u == 0u) ? 0u : 1u;
        out->durations_us[i] = (i % 2u == 0u) ? off_us : on_us;
    }
    return 0;
}
