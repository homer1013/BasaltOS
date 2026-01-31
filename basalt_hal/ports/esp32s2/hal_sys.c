#pragma once
/*
 * BasaltOS Hardware Abstraction Layer - UART
 *
 * Portable UART contract used by BasaltOS.
 *
 * Rules:
 *  - No vendor SDK headers here (ESP-IDF, Pico SDK, STM32 HAL, etc.)
 *  - Consistent error model:
 *      0 / -errno for config calls
 *      bytes / -errno for send/recv calls
 *  - Blocking behavior must be explicit via timeout_ms
 */

#include <stdint.h>
#include <stddef.h>
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------
 * UART configuration types
 * ------------------------------------------------------------ */

typedef enum {
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_EVEN,
    HAL_UART_PARITY_ODD,
} hal_uart_parity_t;

typedef enum {
    HAL_UART_STOP_BITS_1 = 0,
    HAL_UART_STOP_BITS_2,
} hal_uart_stop_bits_t;

typedef enum {
    HAL_UART_DATA_BITS_5 = 5,
    HAL_UART_DATA_BITS_6 = 6,
    HAL_UART_DATA_BITS_7 = 7,
    HAL_UART_DATA_BITS_8 = 8,
} hal_uart_data_bits_t;

typedef enum {
    HAL_UART_FLOW_NONE = 0,
    HAL_UART_FLOW_RTS_CTS,
    HAL_UART_FLOW_XON_XOFF,
} hal_uart_flow_t;

/* ------------------------------------------------------------
 * UART init configuration (optional helper)
 * ------------------------------------------------------------ */

typedef struct {
    uint32_t baud;                 // e.g. 115200
    hal_uart_data_bits_t data_bits;
    hal_uart_stop_bits_t stop_bits;
    hal_uart_parity_t parity;
    hal_uart_flow_t flow;
} hal_uart_config_t;

/* Recommended default config */
static inline hal_uart_config_t hal_uart_config_default(uint32_t baud) {
    hal_uart_config_t c;
    c.baud = baud;
    c.data_bits = HAL_UART_DATA_BITS_8;
    c.stop_bits = HAL_UART_STOP_BITS_1;
    c.parity = HAL_UART_PARITY_NONE;
    c.flow = HAL_UART_FLOW_NONE;
    return c;
}

/* ------------------------------------------------------------
 * API
 * ------------------------------------------------------------ */

/**
 * @brief Initialize a UART peripheral.
 *
 * @param u     UART handle storage (caller-provided)
 * @param bus   Platform UART index (e.g. 0,1,2)
 * @param baud  Baud rate (e.g. 115200)
 *
 * @return 0 on success, -errno on failure
 *
 * Notes:
 *  - Minimal profile required
 *  - Non-blocking configuration call
 */
int hal_uart_init(hal_uart_t *u, int bus, uint32_t baud);

/**
 * @brief Initialize UART with a full configuration (optional).
 *
 * @return 0 on success, -errno on failure
 *
 * Platforms may return -ENOSYS if only basic init is supported.
 */
int hal_uart_init_ex(hal_uart_t *u, int bus, const hal_uart_config_t *cfg);

/**
 * @brief Deinitialize the UART peripheral.
 */
int hal_uart_deinit(hal_uart_t *u);

/**
 * @brief Send bytes over UART.
 *
 * @param timeout_ms  0 = nonblocking attempt
 *                    >0 = block up to timeout
 *                    UINT32_MAX = block "forever" (platform-defined)
 *
 * @return bytes sent on success, -errno on failure
 *
 * Thread-safety:
 *  - ISR: No
 *  - Blocking: Yes (depending on timeout)
 */
int hal_uart_send(hal_uart_t *u,
                  const uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms);

/**
 * @brief Receive bytes over UART.
 *
 * @param timeout_ms  0 = nonblocking attempt
 *                    >0 = block up to timeout
 *                    UINT32_MAX = block "forever" (platform-defined)
 *
 * @return bytes received on success, -errno on failure
 *
 * Thread-safety:
 *  - ISR: No
 *  - Blocking: Yes (depending on timeout)
 */
int hal_uart_recv(hal_uart_t *u,
                  uint8_t *buf,
                  size_t len,
                  uint32_t timeout_ms);

/**
 * @brief Flush the TX path (ensure all bytes are transmitted).
 *
 * @return 0 on success, -errno on failure
 *
 * May block until drained.
 */
int hal_uart_flush(hal_uart_t *u);

/**
 * @brief Query number of bytes available to read without blocking.
 *
 * @param avail receives number of readable bytes
 *
 * @return 0 on success, -errno on failure
 */
int hal_uart_available(hal_uart_t *u, size_t *avail);

/**
 * @brief Set baud rate after initialization.
 */
int hal_uart_set_baud(hal_uart_t *u, uint32_t baud);

/**
 * @brief Configure flow control (RTS/CTS or XON/XOFF) if supported.
 *
 * Platforms that do not support flow control may return -ENOSYS.
 */
int hal_uart_set_flow(hal_uart_t *u, hal_uart_flow_t flow);

/**
 * @brief Send a UART break condition (optional).
 *
 * Platforms that do not support this may return -ENOSYS.
 */
int hal_uart_set_break(hal_uart_t *u, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif
