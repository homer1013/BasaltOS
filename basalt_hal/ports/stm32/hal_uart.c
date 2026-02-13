// BasaltOS STM32 HAL - UART
//
// Minimal runtime contract implementation for:
//   hal/include/hal/hal_uart.h

#include <errno.h>
#include <stdint.h>

#include "hal/hal_uart.h"

typedef struct {
    int bus;
    uint32_t baud;
    hal_uart_flow_t flow;
    int tx_pin;
    int rx_pin;
    int rts_pin;
    int cts_pin;
    int initialized;
} hal_uart_impl_t;

_Static_assert(sizeof(hal_uart_impl_t) <= sizeof(((hal_uart_t *)0)->_opaque),
               "hal_uart_t opaque storage too small for stm32 hal_uart_impl_t");

static inline hal_uart_impl_t *U(hal_uart_t *u) {
    return (hal_uart_impl_t *)u->_opaque;
}

int hal_uart_init(hal_uart_t *u, int bus, uint32_t baud) {
    if (!u || bus < 0 || baud == 0) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    impl->bus = bus;
    impl->baud = baud;
    impl->flow = HAL_UART_FLOW_NONE;
    impl->tx_pin = -1;
    impl->rx_pin = -1;
    impl->rts_pin = -1;
    impl->cts_pin = -1;
    impl->initialized = 1;
    return 0;
}

int hal_uart_init_ex(hal_uart_t *u, int bus, const hal_uart_config_t *cfg) {
    if (!u || !cfg || bus < 0 || cfg->baud == 0) return -EINVAL;
    int rc = hal_uart_init(u, bus, cfg->baud);
    if (rc != 0) return rc;
    hal_uart_impl_t *impl = U(u);
    impl->flow = cfg->flow;
    impl->tx_pin = cfg->tx_pin;
    impl->rx_pin = cfg->rx_pin;
    impl->rts_pin = cfg->rts_pin;
    impl->cts_pin = cfg->cts_pin;
    return 0;
}

int hal_uart_deinit(hal_uart_t *u) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_uart_send(hal_uart_t *u,
                  const uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!u) return -EINVAL;
    if (len > 0 && !buf) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    if (len == 0) return 0;
    return -ENOSYS;
}

int hal_uart_recv(hal_uart_t *u,
                  uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!u) return -EINVAL;
    if (len > 0 && !buf) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    if (len == 0) return 0;
    return -ENOSYS;
}

int hal_uart_flush(hal_uart_t *u) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    return 0;
}

int hal_uart_available(hal_uart_t *u, size_t *avail) {
    if (!u || !avail) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    *avail = 0;
    return 0;
}

int hal_uart_set_baud(hal_uart_t *u, uint32_t baud) {
    if (!u || baud == 0) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    impl->baud = baud;
    return 0;
}

int hal_uart_set_flow(hal_uart_t *u, hal_uart_flow_t flow) {
    if (!u) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    if (flow < HAL_UART_FLOW_NONE || flow > HAL_UART_FLOW_XON_XOFF) return -EINVAL;
    impl->flow = flow;
    return 0;
}

int hal_uart_set_break(hal_uart_t *u, uint32_t duration_ms) {
    (void)duration_ms;
    if (!u) return -EINVAL;
    hal_uart_impl_t *impl = U(u);
    if (!impl->initialized) return -EINVAL;
    return -ENOSYS;
}
