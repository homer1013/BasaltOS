// BasaltOS ESP32C3 HAL - I2S diagnostic backend
//
// This diagnostics HAL uses ESP-IDF I2S std channels (TX+RX) so loopback
// tests exercise the real peripheral path. For loopback validation, wire
// DOUT to DIN externally.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hal/hal_i2s.h"

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "hal_errno.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    int bclk_pin;
    int ws_pin;
    int dout_pin;
    int din_pin;
    int sample_rate_hz;
    int bits_per_sample;
    i2s_chan_handle_t tx;
    i2s_chan_handle_t rx;
    bool tx_ready;
    bool rx_ready;
    bool initialized;
} hal_i2s_impl_t;

_Static_assert(sizeof(hal_i2s_impl_t) <= sizeof(((hal_i2s_t *)0)->_opaque),
               "hal_i2s_t opaque storage too small for esp32 hal_i2s_impl_t");

static inline hal_i2s_impl_t *I(hal_i2s_t *i2s) {
    return (hal_i2s_impl_t *)i2s->_opaque;
}


static i2s_data_bit_width_t map_bits(int bits) {
    if (bits <= 16) return I2S_DATA_BIT_WIDTH_16BIT;
    if (bits <= 24) return I2S_DATA_BIT_WIDTH_24BIT;
    return I2S_DATA_BIT_WIDTH_32BIT;
}

static int bits_to_bytes(int bits) {
    if (bits <= 16) return 2;
    if (bits <= 24) return 3;
    return 4;
}

int hal_i2s_diag_init(hal_i2s_t *i2s,
                      int bclk_pin,
                      int ws_pin,
                      int dout_pin,
                      int din_pin,
                      int sample_rate_hz,
                      int bits_per_sample) {
    if (!i2s) return -EINVAL;
    if (bclk_pin < 0 || ws_pin < 0 || dout_pin < 0 || din_pin < 0) return -EINVAL;

    hal_i2s_impl_t *h = I(i2s);
    memset(h, 0, sizeof(*h));

    h->bclk_pin = bclk_pin;
    h->ws_pin = ws_pin;
    h->dout_pin = dout_pin;
    h->din_pin = din_pin;
    h->sample_rate_hz = (sample_rate_hz > 0) ? sample_rate_hz : 16000;
    h->bits_per_sample = (bits_per_sample > 0) ? bits_per_sample : 16;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = 256;
    chan_cfg.auto_clear = true;

    esp_err_t ret = i2s_new_channel(&chan_cfg, &h->tx, &h->rx);
    if (ret != ESP_OK) {
        return hal_esp_err_to_errno(ret);
    }

    i2s_std_slot_config_t slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        map_bits(h->bits_per_sample), I2S_SLOT_MODE_MONO);
    slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(h->sample_rate_hz),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = h->bclk_pin,
            .ws = h->ws_pin,
            .dout = h->dout_pin,
            .din = h->din_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(h->tx, &std_cfg);
    if (ret != ESP_OK) {
        (void)i2s_del_channel(h->tx);
        (void)i2s_del_channel(h->rx);
        h->tx = NULL;
        h->rx = NULL;
        return hal_esp_err_to_errno(ret);
    }

    ret = i2s_channel_init_std_mode(h->rx, &std_cfg);
    if (ret != ESP_OK) {
        (void)i2s_channel_disable(h->tx);
        (void)i2s_del_channel(h->tx);
        (void)i2s_del_channel(h->rx);
        h->tx = NULL;
        h->rx = NULL;
        return hal_esp_err_to_errno(ret);
    }

    ret = i2s_channel_enable(h->tx);
    if (ret != ESP_OK) {
        (void)i2s_del_channel(h->tx);
        (void)i2s_del_channel(h->rx);
        h->tx = NULL;
        h->rx = NULL;
        return hal_esp_err_to_errno(ret);
    }
    ret = i2s_channel_enable(h->rx);
    if (ret != ESP_OK) {
        (void)i2s_channel_disable(h->tx);
        (void)i2s_del_channel(h->tx);
        (void)i2s_del_channel(h->rx);
        h->tx = NULL;
        h->rx = NULL;
        return hal_esp_err_to_errno(ret);
    }

    h->tx_ready = true;
    h->rx_ready = true;
    h->initialized = true;
    return 0;
}

int hal_i2s_diag_deinit(hal_i2s_t *i2s) {
    if (!i2s) return -EINVAL;
    hal_i2s_impl_t *h = I(i2s);
    if (!h->initialized) return -EINVAL;

    if (h->tx) {
        (void)i2s_channel_disable(h->tx);
    }
    if (h->rx) {
        (void)i2s_channel_disable(h->rx);
    }
    if (h->tx) {
        (void)i2s_del_channel(h->tx);
    }
    if (h->rx) {
        (void)i2s_del_channel(h->rx);
    }

    memset(h, 0, sizeof(*h));
    return 0;
}

int hal_i2s_diag_get_ready(hal_i2s_t *i2s, int *tx_ready_out, int *rx_ready_out) {
    if (!i2s || !tx_ready_out || !rx_ready_out) return -EINVAL;
    hal_i2s_impl_t *h = I(i2s);
    if (!h->initialized) return -EINVAL;
    *tx_ready_out = h->tx_ready ? 1 : 0;
    *rx_ready_out = h->rx_ready ? 1 : 0;
    return 0;
}

int hal_i2s_diag_get_stats(hal_i2s_t *i2s, int *sample_rate_hz_out, int *bits_per_sample_out) {
    if (!i2s) return -EINVAL;
    hal_i2s_impl_t *h = I(i2s);
    if (!h->initialized) return -EINVAL;
    if (sample_rate_hz_out) *sample_rate_hz_out = h->sample_rate_hz;
    if (bits_per_sample_out) *bits_per_sample_out = h->bits_per_sample;
    return 0;
}

static uint32_t us_to_samples(uint32_t us, uint32_t sample_rate_hz) {
    if (us == 0) return 1;
    uint64_t v = ((uint64_t)us * (uint64_t)sample_rate_hz + 500000ULL) / 1000000ULL;
    return (uint32_t)(v == 0 ? 1 : v);
}

static uint32_t samples_to_us(uint32_t samples, uint32_t sample_rate_hz) {
    if (sample_rate_hz == 0) return 0;
    uint64_t v = ((uint64_t)samples * 1000000ULL + (sample_rate_hz / 2ULL)) / (uint64_t)sample_rate_hz;
    return (uint32_t)v;
}

int hal_i2s_diag_loopback(hal_i2s_t *i2s,
                          uint32_t on_us,
                          uint32_t off_us,
                          uint32_t count,
                          uint32_t window_ms,
                          uint32_t poll_us,
                          hal_i2s_diag_capture_t *out) {
    (void)poll_us;
    if (!i2s || !out) return -EINVAL;
    if (count == 0 || count > 10000) return -EINVAL;
    if (window_ms < 10 || window_ms > 10000) return -EINVAL;

    hal_i2s_impl_t *h = I(i2s);
    if (!h->initialized || !h->tx_ready || !h->rx_ready || !h->tx || !h->rx) return -EINVAL;

    const uint32_t sample_rate = (uint32_t)h->sample_rate_hz;
    const uint32_t on_samples = us_to_samples(on_us, sample_rate);
    const uint32_t off_samples = us_to_samples(off_us, sample_rate);
    const uint64_t total_samples64 = (uint64_t)count * (uint64_t)(on_samples + off_samples);
    if (total_samples64 == 0 || total_samples64 > 200000ULL) {
        return -E2BIG;
    }

    const uint32_t total_samples = (uint32_t)total_samples64;
    const int sample_bytes = bits_to_bytes(h->bits_per_sample);
    const size_t frame_bytes = (size_t)sample_bytes * 2u;
    const size_t total_bytes = (size_t)total_samples * frame_bytes;

    uint8_t *tx_buf = (uint8_t *)calloc(1, total_bytes);
    uint8_t *rx_buf = (uint8_t *)calloc(1, total_bytes);
    if (!tx_buf || !rx_buf) {
        free(tx_buf);
        free(rx_buf);
        return -ENOMEM;
    }

    uint32_t idx = 0;
    const int32_t hi = 12000;
    const int32_t lo = -12000;
    for (uint32_t c = 0; c < count; ++c) {
        for (uint32_t j = 0; j < on_samples && idx < total_samples; ++j, ++idx) {
            int16_t v = (int16_t)hi;
            memcpy(&tx_buf[idx * frame_bytes], &v, sizeof(v));
            memcpy(&tx_buf[idx * frame_bytes + sample_bytes], &v, sizeof(v));
        }
        for (uint32_t j = 0; j < off_samples && idx < total_samples; ++j, ++idx) {
            int16_t v = (int16_t)lo;
            memcpy(&tx_buf[idx * frame_bytes], &v, sizeof(v));
            memcpy(&tx_buf[idx * frame_bytes + sample_bytes], &v, sizeof(v));
        }
    }

    const TickType_t timeout_ticks = pdMS_TO_TICKS(window_ms);
    size_t written = 0;
    esp_err_t ret = i2s_channel_write(h->tx, tx_buf, total_bytes, &written, timeout_ticks);
    if (ret != ESP_OK) {
        free(tx_buf);
        free(rx_buf);
        return hal_esp_err_to_errno(ret);
    }

    size_t read = 0;
    ret = i2s_channel_read(h->rx, rx_buf, written, &read, timeout_ticks);
    if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
        free(tx_buf);
        free(rx_buf);
        return hal_esp_err_to_errno(ret);
    }

    memset(out, 0, sizeof(*out));

    uint32_t samples_read = (uint32_t)(read / frame_bytes);
    if (samples_read == 0) {
        out->start_level = 0;
        out->levels[0] = 0;
        free(tx_buf);
        free(rx_buf);
        return 0;
    }

    int16_t first = 0;
    memcpy(&first, &rx_buf[0], sizeof(first));
    int cur_level = (first > 0) ? 1 : 0;
    out->start_level = cur_level;
    out->levels[0] = (uint32_t)cur_level;

    uint32_t run_len = 0;
    uint32_t edges = 0;
    for (uint32_t s = 0; s < samples_read; ++s) {
        int16_t v = 0;
        memcpy(&v, &rx_buf[s * frame_bytes], sizeof(v));
        int level = (v > 0) ? 1 : 0;
        if (level == cur_level) {
            run_len++;
            continue;
        }
        if (edges < HAL_I2S_DIAG_MAX_EDGES) {
            out->durations_us[edges] = samples_to_us(run_len, sample_rate);
            edges++;
            out->levels[edges] = (uint32_t)level;
        }
        cur_level = level;
        run_len = 1;
    }
    out->durations_us[edges] = samples_to_us(run_len, sample_rate);
    out->edges = edges;

    free(tx_buf);
    free(rx_buf);
    return 0;
}
