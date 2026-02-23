// BasaltOS ESP32C6 HAL - UART
//
// ESP-IDF implementation of:
//   hal/include/hal/hal_uart.h
//
// Return conventions:
//   0 / -errno for config calls
//   bytes / -errno for send/recv calls

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_err.h"
#include "hal_errno.h"

// -----------------------------------------------------------------------------
// Private implementation type stored inside hal_uart_t opaque storage
// -----------------------------------------------------------------------------

typedef struct {
    uart_port_t port;
    uint32_t baud;
    bool driver_owner;
    bool initialized;
    hal_uart_flow_t flow;

    int tx_pin;
    int rx_pin;
    int rts_pin;
    int cts_pin;
} hal_uart_impl_t;

// Ensure the public opaque handle has enough storage for this port's impl.
// hal_uart_t is defined in hal_types.h as: typedef struct { uint32_t _opaque[N]; } hal_uart_t;
_Static_assert(sizeof(hal_uart_impl_t) <= sizeof(((hal_uart_t *)0)->_opaque),
               "hal_uart_t opaque storage too small for esp32 hal_uart_impl_t");

static inline hal_uart_impl_t *U(hal_uart_t *u) {
    return (hal_uart_impl_t *)u->_opaque;
}
static inline const hal_uart_impl_t *UC(const hal_uart_t *u) {
    return (const hal_uart_impl_t *)u->_opaque;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static inline TickType_t ms_to_ticks(uint32_t timeout_ms) {
    if (timeout_ms == 0) return 0;
    if (timeout_ms == UINT32_MAX) return portMAX_DELAY;
    return pdMS_TO_TICKS(timeout_ms);
}

static uart_parity_t map_parity(hal_uart_parity_t p) {
    switch (p) {
        case HAL_UART_PARITY_NONE: return UART_PARITY_DISABLE;
        case HAL_UART_PARITY_EVEN: return UART_PARITY_EVEN;
        case HAL_UART_PARITY_ODD:  return UART_PARITY_ODD;
        default:                   return UART_PARITY_DISABLE;
    }
}

static uart_stop_bits_t map_stop_bits(hal_uart_stop_bits_t s) {
    switch (s) {
        case HAL_UART_STOP_BITS_1: return UART_STOP_BITS_1;
        case HAL_UART_STOP_BITS_2: return UART_STOP_BITS_2;
        default:                   return UART_STOP_BITS_1;
    }
}

static uart_word_length_t map_data_bits(hal_uart_data_bits_t d) {
    switch (d) {
        case HAL_UART_DATA_BITS_5: return UART_DATA_5_BITS;
        case HAL_UART_DATA_BITS_6: return UART_DATA_6_BITS;
        case HAL_UART_DATA_BITS_7: return UART_DATA_7_BITS;
        case HAL_UART_DATA_BITS_8: return UART_DATA_8_BITS;
        default:                   return UART_DATA_8_BITS;
    }
}

static inline int pin_or_nochange(int pin) {
    return (pin < 0) ? UART_PIN_NO_CHANGE : pin;
}

static int apply_pins(hal_uart_impl_t *u, const hal_uart_config_t *cfg) {
    // Apply routing only if any pin is specified.
    if (cfg->tx_pin < 0 && cfg->rx_pin < 0 && cfg->rts_pin < 0 && cfg->cts_pin < 0) {
        // Leave defaults as-is.
        u->tx_pin = -1;
        u->rx_pin = -1;
        u->rts_pin = -1;
        u->cts_pin = -1;
        return 0;
    }

    esp_err_t e = uart_set_pin(
        u->port,
        pin_or_nochange(cfg->tx_pin),
        pin_or_nochange(cfg->rx_pin),
        pin_or_nochange(cfg->rts_pin),
        pin_or_nochange(cfg->cts_pin)
    );
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    u->tx_pin  = cfg->tx_pin;
    u->rx_pin  = cfg->rx_pin;
    u->rts_pin = cfg->rts_pin;
    u->cts_pin = cfg->cts_pin;
    return 0;
}

static int apply_flow_ctrl(hal_uart_impl_t *u, hal_uart_flow_t flow) {
    esp_err_t e;

    switch (flow) {
        case HAL_UART_FLOW_NONE:
            e = uart_set_hw_flow_ctrl(u->port, UART_HW_FLOWCTRL_DISABLE, 0);
            if (e != ESP_OK) return hal_esp_err_to_errno(e);
#ifdef uart_set_sw_flow_ctrl
            (void)uart_set_sw_flow_ctrl(u->port, false, 0, 0);
#endif
            u->flow = flow;
            return 0;

        case HAL_UART_FLOW_RTS_CTS:
            // NOTE: For RTS/CTS to actually work, the pins must be routed.
            // If user didn't set pins, this still "enables" HW flow, but you may
            // not have RTS/CTS connected. That’s fine; it’s a hardware wiring issue.
            e = uart_set_hw_flow_ctrl(u->port, UART_HW_FLOWCTRL_CTS_RTS, 122);
            if (e != ESP_OK) return hal_esp_err_to_errno(e);
            u->flow = flow;
            return 0;

        case HAL_UART_FLOW_XON_XOFF:
#ifdef uart_set_sw_flow_ctrl
            e = uart_set_sw_flow_ctrl(u->port, true, 80, 20);
            if (e != ESP_OK) return hal_esp_err_to_errno(e);
            (void)uart_set_hw_flow_ctrl(u->port, UART_HW_FLOWCTRL_DISABLE, 0);
            u->flow = flow;
            return 0;
#else
            (void)flow;
            return -ENOSYS;
#endif

        default:
            return -EINVAL;
    }
}

static int uart_install_if_needed(hal_uart_impl_t *u) {
    // Required for uart_read_bytes/uart_write_bytes.
    const int rx_buf_sz = 2048;
    const int tx_buf_sz = 2048;

    esp_err_t e = uart_driver_install(u->port, rx_buf_sz, tx_buf_sz, 0, NULL, 0);
    if (e == ESP_OK) {
        u->driver_owner = true;
    } else if (e == ESP_ERR_INVALID_STATE) {
        u->driver_owner = false;
    } else {
        return hal_esp_err_to_errno(e);
    }
    return 0;
}

static void uart_rollback_driver_if_owned(hal_uart_impl_t *u) {
    if (u->driver_owner) {
        (void)uart_driver_delete(u->port);
        u->driver_owner = false;
    }
}

static int uart_apply_config(hal_uart_impl_t *u, const hal_uart_config_t *cfg) {
    uart_config_t c = {0};
    c.baud_rate = (int)cfg->baud;
    c.data_bits = map_data_bits(cfg->data_bits);
    c.parity    = map_parity(cfg->parity);
    c.stop_bits = map_stop_bits(cfg->stop_bits);
    c.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    c.source_clk = UART_SCLK_DEFAULT;

    esp_err_t e = uart_param_config(u->port, &c);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    int rc = uart_install_if_needed(u);
    if (rc != 0) return rc;

    // Apply pin routing (optional)
    rc = apply_pins(u, cfg);
    if (rc != 0) {
        uart_rollback_driver_if_owned(u);
        return rc;
    }

    // Apply flow control
    rc = apply_flow_ctrl(u, cfg->flow);
    if (rc != 0) {
        uart_rollback_driver_if_owned(u);
        return rc;
    }

    u->baud = cfg->baud;
    return 0;
}

// -----------------------------------------------------------------------------
// Public API (matches hal_uart.h exactly)
// -----------------------------------------------------------------------------

int hal_uart_init(hal_uart_t *u, int bus, uint32_t baud) {
    if (!u) return -EINVAL;
    if (bus < 0) return -EINVAL;
    if (baud == 0) return -EINVAL;

    hal_uart_impl_t *iu = U(u);

    iu->port = (uart_port_t)bus;
    iu->driver_owner = false;
    iu->initialized = false;

    hal_uart_config_t cfg = hal_uart_config_default(baud);
    // No pin routing in basic init() by default (keeps classic behavior).

    int rc = uart_apply_config(iu, &cfg);
    if (rc != 0) return rc;

    iu->initialized = true;
    return 0;
}

int hal_uart_init_ex(hal_uart_t *u, int bus, const hal_uart_config_t *cfg) {
    if (!u || !cfg) return -EINVAL;
    if (bus < 0) return -EINVAL;
    if (cfg->baud == 0) return -EINVAL;

    hal_uart_impl_t *iu = U(u);

    iu->port = (uart_port_t)bus;
    iu->driver_owner = false;
    iu->initialized = false;

    int rc = uart_apply_config(iu, cfg);
    if (rc != 0) return rc;

    iu->initialized = true;
    return 0;
}

int hal_uart_deinit(hal_uart_t *u) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;

    esp_err_t e = ESP_OK;
    if (iu->driver_owner) {
        e = uart_driver_delete(iu->port);
    }
    iu->driver_owner = false;
    iu->initialized = false;
    return hal_esp_err_to_errno(e);
}

int hal_uart_send(hal_uart_t *u,
                  const uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;
    if (!buf && len > 0) return -EINVAL;
    if (len > (size_t)INT_MAX) return -EMSGSIZE;


    int written = uart_write_bytes(iu->port, (const char *)buf, (int)len);
    if (written < 0) return -EIO;

    if (timeout_ms != 0) {
        esp_err_t e = uart_wait_tx_done(iu->port, ms_to_ticks(timeout_ms));
        if (e != ESP_OK) return hal_esp_err_to_errno(e);
    }

    return written;
}

int hal_uart_recv(hal_uart_t *u,
                  uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;
    if (!buf && len > 0) return -EINVAL;
    if (len > (size_t)INT_MAX) return -EMSGSIZE;


    int rd = uart_read_bytes(iu->port, buf, (uint32_t)len, ms_to_ticks(timeout_ms));
    if (rd < 0) return -EIO;
    return rd;
}

int hal_uart_flush(hal_uart_t *u) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;

    esp_err_t e = uart_wait_tx_done(iu->port, portMAX_DELAY);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    // Flush RX as well (common expectation of "flush")
    e = uart_flush_input(iu->port);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    return 0;
}

int hal_uart_available(hal_uart_t *u, size_t *avail) {
    if (!u || !avail) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;

    size_t n = 0;
    esp_err_t e = uart_get_buffered_data_len(iu->port, &n);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    *avail = n;
    return 0;
}

int hal_uart_set_baud(hal_uart_t *u, uint32_t baud) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;
    if (baud == 0) return -EINVAL;

    esp_err_t e = uart_set_baudrate(iu->port, (int)baud);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    iu->baud = baud;
    return 0;
}

int hal_uart_set_flow(hal_uart_t *u, hal_uart_flow_t flow) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;
    return apply_flow_ctrl(iu, flow);
}

int hal_uart_set_break(hal_uart_t *u, uint32_t duration_ms) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *iu = U(u);
    if (!iu->initialized) return -EINVAL;
    if (duration_ms == 0) return 0;

    uint32_t bits = (iu->baud == 0) ? 1u : (uint32_t)((((uint64_t)iu->baud) * duration_ms) / 1000ULL);
    if (bits == 0) bits = 1;

#ifdef uart_tx_break
    esp_err_t e = uart_tx_break(iu->port, (int)bits);
    return hal_esp_err_to_errno(e);
#elif defined(uart_set_break)
    esp_err_t e = uart_set_break(iu->port, (int)bits);
    return hal_esp_err_to_errno(e);
#else
    (void)bits;
    return -ENOSYS;
#endif
}
