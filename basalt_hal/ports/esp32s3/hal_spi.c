// BasaltOS ESP32-s3 HAL - SPI

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_spi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "esp_err.h"
#include "hal_errno.h"

typedef struct {
    spi_host_device_t host;
    spi_device_handle_t dev;
    uint32_t freq_hz;
    int sclk_pin;
    int mosi_pin;
    int miso_pin;
    int cs_pin;
    hal_spi_mode_t mode;
    bool initialized;
    bool bus_owner;
} hal_spi_impl_t;

_Static_assert(sizeof(hal_spi_impl_t) <= sizeof(((hal_spi_t *)0)->_opaque),
               "hal_spi_t opaque storage too small for esp32s3 hal_spi_impl_t");

static inline hal_spi_impl_t *S(hal_spi_t *spi) {
    return (hal_spi_impl_t *)spi->_opaque;
}

static inline TickType_t ms_to_ticks(uint32_t timeout_ms) {
    if (timeout_ms == 0) return 0;
    if (timeout_ms == UINT32_MAX) return portMAX_DELAY;
    return pdMS_TO_TICKS(timeout_ms);
}

int hal_spi_init(hal_spi_t *spi,
                 int bus,
                 uint32_t freq_hz,
                 int sclk_pin,
                 int mosi_pin,
                 int miso_pin,
                 int cs_pin,
                 hal_spi_mode_t mode) {
    if (!spi || bus < 0 || freq_hz == 0 || sclk_pin < 0 || cs_pin < 0) return -EINVAL;

    hal_spi_impl_t *s = S(spi);
    s->host = (spi_host_device_t)bus;
    s->freq_hz = freq_hz;
    s->sclk_pin = sclk_pin;
    s->mosi_pin = mosi_pin;
    s->miso_pin = miso_pin;
    s->cs_pin = cs_pin;
    s->mode = mode;
    s->dev = NULL;
    s->bus_owner = false;
    s->initialized = false;

    spi_bus_config_t bus_cfg = {0};
    bus_cfg.sclk_io_num = sclk_pin;
    bus_cfg.mosi_io_num = mosi_pin;
    bus_cfg.miso_io_num = miso_pin;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4096;

    esp_err_t e = spi_bus_initialize(s->host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (e == ESP_OK) {
        s->bus_owner = true;
    } else if (e != ESP_ERR_INVALID_STATE) {
        return hal_esp_err_to_errno(e);
    }

    spi_device_interface_config_t dev_cfg = {0};
    dev_cfg.clock_speed_hz = (int)freq_hz;
    dev_cfg.mode = (uint8_t)mode;
    dev_cfg.spics_io_num = cs_pin;
    dev_cfg.queue_size = 1;
    dev_cfg.flags = 0;

    e = spi_bus_add_device(s->host, &dev_cfg, &s->dev);
    if (e != ESP_OK) {
        if (s->bus_owner) {
            (void)spi_bus_free(s->host);
            s->bus_owner = false;
        }
        return hal_esp_err_to_errno(e);
    }

    s->initialized = true;
    return 0;
}

int hal_spi_deinit(hal_spi_t *spi) {
    if (!spi) return -EINVAL;
    hal_spi_impl_t *s = S(spi);
    if (!s->initialized) return -EINVAL;

    if (s->dev) {
        (void)spi_bus_remove_device(s->dev);
        s->dev = NULL;
    }
    if (s->bus_owner) {
        (void)spi_bus_free(s->host);
        s->bus_owner = false;
    }
    s->initialized = false;
    return 0;
}

int hal_spi_set_freq(hal_spi_t *spi, uint32_t freq_hz) {
    if (!spi || freq_hz == 0) return -EINVAL;
    hal_spi_impl_t *s = S(spi);
    if (!s->initialized) return -EINVAL;

    spi_device_interface_config_t cfg = {0};
    cfg.clock_speed_hz = (int)freq_hz;
    cfg.mode = (uint8_t)s->mode;
    cfg.spics_io_num = s->cs_pin;
    cfg.queue_size = 1;
    cfg.flags = 0;

    spi_device_handle_t new_dev = NULL;
    esp_err_t e = spi_bus_remove_device(s->dev);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);
    s->dev = NULL;

    e = spi_bus_add_device(s->host, &cfg, &new_dev);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    s->dev = new_dev;
    s->freq_hz = freq_hz;
    return 0;
}

int hal_spi_set_mode(hal_spi_t *spi, hal_spi_mode_t mode) {
    if (!spi) return -EINVAL;
    hal_spi_impl_t *s = S(spi);
    if (!s->initialized) return -EINVAL;

    spi_device_interface_config_t cfg = {0};
    cfg.clock_speed_hz = (int)s->freq_hz;
    cfg.mode = (uint8_t)mode;
    cfg.spics_io_num = s->cs_pin;
    cfg.queue_size = 1;
    cfg.flags = 0;

    spi_device_handle_t new_dev = NULL;
    esp_err_t e = spi_bus_remove_device(s->dev);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);
    s->dev = NULL;

    e = spi_bus_add_device(s->host, &cfg, &new_dev);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    s->dev = new_dev;
    s->mode = mode;
    return 0;
}

int hal_spi_transfer(hal_spi_t *spi,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len,
                     uint32_t timeout_ms) {
    if (!spi) return -EINVAL;
    hal_spi_impl_t *s = S(spi);
    if (!s->initialized || !s->dev) return -EINVAL;
    if (len == 0) return 0;
    if (!tx && !rx) return -EINVAL;
    if (len > ((size_t)INT_MAX / 8U)) return -EMSGSIZE;

    spi_transaction_t t = {0};
    t.length = (int)(len * 8);
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t e = spi_device_polling_transmit(s->dev, &t);
    if (e == ESP_OK) return (int)len;
    if (e == ESP_ERR_TIMEOUT) {
        (void)ms_to_ticks(timeout_ms);
        return -ETIMEDOUT;
    }
    return hal_esp_err_to_errno(e);
}

int hal_spi_write(hal_spi_t *spi,
                  const uint8_t *tx,
                  size_t len,
                  uint32_t timeout_ms) {
    return hal_spi_transfer(spi, tx, NULL, len, timeout_ms);
}

int hal_spi_read(hal_spi_t *spi,
                 uint8_t *rx,
                 size_t len,
                 uint32_t timeout_ms) {
    return hal_spi_transfer(spi, NULL, rx, len, timeout_ms);
}
