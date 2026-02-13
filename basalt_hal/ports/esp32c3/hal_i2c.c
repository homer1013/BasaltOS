// BasaltOS ESP32C3 HAL - I2C
//
// ESP-IDF implementation of:
//   hal/include/hal/hal_i2c.h

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_err.h"

// -----------------------------------------------------------------------------
// Private implementation type stored inside hal_i2c_t opaque storage
// -----------------------------------------------------------------------------

typedef struct {
    int port;            // i2c_port_t but stored as int to keep public headers clean
    uint32_t freq_hz;
    int sda_pin;
    int scl_pin;
    bool initialized;
} hal_i2c_impl_t;

_Static_assert(sizeof(hal_i2c_impl_t) <= sizeof(((hal_i2c_t *)0)->_opaque),
               "hal_i2c_t opaque storage too small for esp32 hal_i2c_impl_t");

static inline hal_i2c_impl_t *I(hal_i2c_t *i2c) {
    return (hal_i2c_impl_t *)i2c->_opaque;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

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

static inline TickType_t ms_to_ticks(uint32_t timeout_ms) {
    // If timeout_ms==0, let the driver do a "no wait" attempt where supported.
    return (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
}

static inline bool valid_addr7(uint8_t addr) {
    return (addr <= 0x7F);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

int hal_i2c_init(hal_i2c_t *i2c, int bus, uint32_t freq_hz, int sda_pin, int scl_pin) {
    if (!i2c) return -EINVAL;
    if (bus < 0) return -EINVAL;
    if (freq_hz == 0) return -EINVAL;
    if (sda_pin < 0 || scl_pin < 0) return -EINVAL;

    hal_i2c_impl_t *h = I(i2c);

    h->port = bus;
    h->freq_hz = freq_hz;
    h->sda_pin = sda_pin;
    h->scl_pin = scl_pin;

    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)sda_pin;
    conf.scl_io_num = (gpio_num_t)scl_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq_hz;

    esp_err_t e1 = i2c_param_config((i2c_port_t)bus, &conf);
    if (e1 != ESP_OK) return esp_err_to_errno(e1);

    // Last two args are RX/TX buffer sizes for slave mode; set 0 for master mode.
    esp_err_t e2 = i2c_driver_install((i2c_port_t)bus, I2C_MODE_MASTER, 0, 0, 0);
    if (e2 != ESP_OK && e2 != ESP_ERR_INVALID_STATE) {
        return esp_err_to_errno(e2);
    }

    h->initialized = true;
    return 0;
}

int hal_i2c_deinit(hal_i2c_t *i2c) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;

    esp_err_t e = i2c_driver_delete((i2c_port_t)h->port);
    h->initialized = false;
    return esp_err_to_errno(e);
}

int hal_i2c_set_freq(hal_i2c_t *i2c, uint32_t freq_hz) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;
    if (freq_hz == 0) return -EINVAL;

    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)h->sda_pin;
    conf.scl_io_num = (gpio_num_t)h->scl_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq_hz;

    esp_err_t e = i2c_param_config((i2c_port_t)h->port, &conf);
    if (e == ESP_OK) {
        h->freq_hz = freq_hz;
        return 0;
    }
    return esp_err_to_errno(e);
}

int hal_i2c_probe(hal_i2c_t *i2c, uint8_t addr, uint32_t timeout_ms) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;
    if (!valid_addr7(addr)) return -EINVAL;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) return -ENOMEM;

    esp_err_t e = ESP_OK;
    e |= i2c_master_start(cmd);
    e |= i2c_master_write_byte(cmd, (uint8_t)((addr << 1) | I2C_MASTER_WRITE), true);
    e |= i2c_master_stop(cmd);

    if (e != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        return esp_err_to_errno(e);
    }

    esp_err_t ex = i2c_master_cmd_begin((i2c_port_t)h->port, cmd, ms_to_ticks(timeout_ms));
    i2c_cmd_link_delete(cmd);

    if (ex == ESP_OK) return 0;

    if (ex == ESP_ERR_TIMEOUT) return -ETIMEDOUT;
    if (ex == ESP_ERR_INVALID_STATE) return -EALREADY;
    if (ex == ESP_ERR_INVALID_ARG) return -EINVAL;

    return -ENODEV;
}

int hal_i2c_write(hal_i2c_t *i2c, uint8_t addr,
                  const uint8_t *data, size_t len,
                  uint32_t timeout_ms) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;
    if (!valid_addr7(addr)) return -EINVAL;
    if (!data && len > 0) return -EINVAL;

    esp_err_t e = i2c_master_write_to_device(
        (i2c_port_t)h->port,
        addr,
        (uint8_t *)data,
        len,
        ms_to_ticks(timeout_ms)
    );

    if (e != ESP_OK) return esp_err_to_errno(e);
    return (int)len;
}

int hal_i2c_read(hal_i2c_t *i2c, uint8_t addr,
                 uint8_t *data, size_t len,
                 uint32_t timeout_ms) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;
    if (!valid_addr7(addr)) return -EINVAL;
    if (!data && len > 0) return -EINVAL;

    esp_err_t e = i2c_master_read_from_device(
        (i2c_port_t)h->port,
        addr,
        data,
        len,
        ms_to_ticks(timeout_ms)
    );

    if (e != ESP_OK) return esp_err_to_errno(e);
    return (int)len;
}

int hal_i2c_write_read(hal_i2c_t *i2c, uint8_t addr,
                       const uint8_t *wdata, size_t wlen,
                       uint8_t *rdata, size_t rlen,
                       uint32_t timeout_ms) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *h = I(i2c);
    if (!h->initialized) return -EINVAL;
    if (!valid_addr7(addr)) return -EINVAL;
    if (!wdata && wlen > 0) return -EINVAL;
    if (!rdata && rlen > 0) return -EINVAL;

    esp_err_t e = i2c_master_write_read_device(
        (i2c_port_t)h->port,
        addr,
        (uint8_t *)wdata,
        wlen,
        rdata,
        rlen,
        ms_to_ticks(timeout_ms)
    );

    if (e != ESP_OK) return esp_err_to_errno(e);
    return (int)rlen;
}
