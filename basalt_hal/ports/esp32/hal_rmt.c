// BasaltOS ESP32 HAL - RMT diagnostics backend
//
// This implementation intentionally uses GPIO-level pulse/capture timing in the
// HAL layer to keep app/shell logic out of board-specific pin orchestration.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hal/hal_rmt.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

typedef struct {
    int tx_pin;
    int rx_pin;
    uint32_t resolution_hz;
    bool enable_tx;
    bool enable_rx;
    bool tx_ready;
    bool rx_ready;
    bool initialized;
} hal_rmt_impl_t;

_Static_assert(sizeof(hal_rmt_impl_t) <= sizeof(((hal_rmt_t *)0)->_opaque),
               "hal_rmt_t opaque storage too small for esp32 hal_rmt_impl_t");

static inline hal_rmt_impl_t *R(hal_rmt_t *rmt) {
    return (hal_rmt_impl_t *)rmt->_opaque;
}

static inline int esp_err_to_errno(esp_err_t err) {
    switch (err) {
        case ESP_OK: return 0;
        case ESP_ERR_INVALID_ARG: return -EINVAL;
        case ESP_ERR_INVALID_STATE: return -EALREADY;
        case ESP_ERR_NO_MEM: return -ENOMEM;
        case ESP_ERR_TIMEOUT: return -ETIMEDOUT;
        default: return -EIO;
    }
}

static int hal_capture_gpio(int pin,
                            uint32_t window_ms,
                            uint32_t poll_us,
                            hal_rmt_capture_t *out) {
    if (!out) return -EINVAL;
    memset(out, 0, sizeof(*out));
    if (pin < 0) return -EINVAL;
    if (window_ms < 10 || window_ms > 10000) return -EINVAL;
    if (poll_us == 0) poll_us = 25;

    int64_t start_us = esp_timer_get_time();
    int64_t end_us = start_us + ((int64_t)window_ms * 1000LL);
    int last_level = gpio_get_level((gpio_num_t)pin) ? 1 : 0;
    int64_t last_edge_us = start_us;
    uint32_t edges = 0;

    out->start_level = last_level;
    out->levels[0] = (uint32_t)last_level;

    while (esp_timer_get_time() < end_us && edges < HAL_RMT_CAPTURE_MAX_EDGES) {
        int level = gpio_get_level((gpio_num_t)pin) ? 1 : 0;
        int64_t now_us = esp_timer_get_time();
        if (level != last_level) {
            out->durations_us[edges] = (uint32_t)(now_us - last_edge_us);
            edges++;
            out->levels[edges] = (uint32_t)level;
            last_level = level;
            last_edge_us = now_us;
        }
        esp_rom_delay_us(poll_us);
    }

    {
        int64_t done_us = esp_timer_get_time();
        if (done_us > last_edge_us) {
            out->durations_us[edges] = (uint32_t)(done_us - last_edge_us);
        }
    }
    out->edges = edges;
    return 0;
}

int hal_rmt_init(hal_rmt_t *rmt,
                 int tx_pin,
                 int rx_pin,
                 uint32_t resolution_hz,
                 int enable_tx,
                 int enable_rx) {
    if (!rmt) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);

    h->tx_pin = tx_pin;
    h->rx_pin = rx_pin;
    h->resolution_hz = resolution_hz ? resolution_hz : 1000000u;
    h->enable_tx = (enable_tx != 0);
    h->enable_rx = (enable_rx != 0);
    h->tx_ready = false;
    h->rx_ready = false;
    h->initialized = false;

    if (h->enable_tx) {
        if (h->tx_pin < 0) return -EINVAL;
        esp_err_t ret = gpio_reset_pin((gpio_num_t)h->tx_pin);
        if (ret != ESP_OK) return esp_err_to_errno(ret);
        ret = gpio_set_direction((gpio_num_t)h->tx_pin, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) return esp_err_to_errno(ret);
        (void)gpio_set_level((gpio_num_t)h->tx_pin, 0);
        h->tx_ready = true;
    }
    if (h->enable_rx) {
        if (h->rx_pin < 0) return -EINVAL;
        esp_err_t ret = gpio_reset_pin((gpio_num_t)h->rx_pin);
        if (ret != ESP_OK) return esp_err_to_errno(ret);
        ret = gpio_set_direction((gpio_num_t)h->rx_pin, GPIO_MODE_INPUT);
        if (ret != ESP_OK) return esp_err_to_errno(ret);
        h->rx_ready = true;
    }
    h->initialized = true;
    return 0;
}

int hal_rmt_deinit(hal_rmt_t *rmt) {
    if (!rmt) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);
    if (!h->initialized) return -EINVAL;

    if (h->tx_ready) (void)gpio_reset_pin((gpio_num_t)h->tx_pin);
    if (h->rx_ready) (void)gpio_reset_pin((gpio_num_t)h->rx_pin);
    h->initialized = false;
    h->tx_ready = false;
    h->rx_ready = false;
    return 0;
}

int hal_rmt_get_ready(hal_rmt_t *rmt, int *tx_ready_out, int *rx_ready_out) {
    if (!rmt || !tx_ready_out || !rx_ready_out) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);
    if (!h->initialized) return -EINVAL;
    *tx_ready_out = h->tx_ready ? 1 : 0;
    *rx_ready_out = h->rx_ready ? 1 : 0;
    return 0;
}

int hal_rmt_pulse(hal_rmt_t *rmt, uint32_t on_us, uint32_t off_us, uint32_t count) {
    if (!rmt) return -EINVAL;
    if (count == 0 || count > 10000) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);
    if (!h->initialized || !h->tx_ready) return -EINVAL;

    for (uint32_t i = 0; i < count; ++i) {
        gpio_set_level((gpio_num_t)h->tx_pin, 1);
        if (on_us) esp_rom_delay_us(on_us);
        gpio_set_level((gpio_num_t)h->tx_pin, 0);
        if (off_us) esp_rom_delay_us(off_us);
    }
    return 0;
}

int hal_rmt_capture(hal_rmt_t *rmt,
                    uint32_t window_ms,
                    uint32_t poll_us,
                    hal_rmt_capture_t *out) {
    if (!rmt || !out) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);
    if (!h->initialized || !h->rx_ready) return -EINVAL;
    return hal_capture_gpio(h->rx_pin, window_ms, poll_us, out);
}

int hal_rmt_loopback(hal_rmt_t *rmt,
                     uint32_t on_us,
                     uint32_t off_us,
                     uint32_t count,
                     uint32_t window_ms,
                     uint32_t poll_us,
                     hal_rmt_capture_t *out) {
    if (!rmt || !out) return -EINVAL;
    if (count == 0 || count > 10000) return -EINVAL;
    hal_rmt_impl_t *h = R(rmt);
    if (!h->initialized || !h->tx_ready || !h->rx_ready) return -EINVAL;
    if (window_ms < 10 || window_ms > 10000) return -EINVAL;
    if (poll_us == 0) poll_us = 25;

    memset(out, 0, sizeof(*out));
    int64_t start_us = esp_timer_get_time();
    int64_t end_us = start_us + ((int64_t)window_ms * 1000LL);
    int last_level = gpio_get_level((gpio_num_t)h->rx_pin) ? 1 : 0;
    int64_t last_edge_us = start_us;
    uint32_t edges = 0;
    uint32_t emitted = 0;

    out->start_level = last_level;
    out->levels[0] = (uint32_t)last_level;

    while (esp_timer_get_time() < end_us && (emitted < count || edges < HAL_RMT_CAPTURE_MAX_EDGES)) {
        if (emitted < count) {
            gpio_set_level((gpio_num_t)h->tx_pin, 1);
            esp_rom_delay_us(3);
            {
                int level = gpio_get_level((gpio_num_t)h->rx_pin) ? 1 : 0;
                int64_t now_us = esp_timer_get_time();
                if (level != last_level && edges < HAL_RMT_CAPTURE_MAX_EDGES) {
                    out->durations_us[edges] = (uint32_t)(now_us - last_edge_us);
                    edges++;
                    out->levels[edges] = (uint32_t)level;
                    last_level = level;
                    last_edge_us = now_us;
                }
            }
            if (on_us) esp_rom_delay_us(on_us);

            gpio_set_level((gpio_num_t)h->tx_pin, 0);
            esp_rom_delay_us(3);
            {
                int level = gpio_get_level((gpio_num_t)h->rx_pin) ? 1 : 0;
                int64_t now_us = esp_timer_get_time();
                if (level != last_level && edges < HAL_RMT_CAPTURE_MAX_EDGES) {
                    out->durations_us[edges] = (uint32_t)(now_us - last_edge_us);
                    edges++;
                    out->levels[edges] = (uint32_t)level;
                    last_level = level;
                    last_edge_us = now_us;
                }
            }
            if (off_us) esp_rom_delay_us(off_us);
            emitted++;
        } else {
            esp_rom_delay_us(poll_us);
        }
        if (edges >= HAL_RMT_CAPTURE_MAX_EDGES && emitted >= count) break;
    }

    {
        int64_t done_us = esp_timer_get_time();
        if (done_us > last_edge_us) {
            out->durations_us[edges] = (uint32_t)(done_us - last_edge_us);
        }
    }
    out->edges = edges;
    return 0;
}
