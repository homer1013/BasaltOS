#pragma once
/*
 * BasaltOS Hardware Abstraction Layer - Common Types
 *
 * This header provides shared types and conventions used across the HAL.
 *
 * Key design direction:
 *  - HAL handles are stack-allocatable.
 *  - Handles are opaque storage blobs that ports reinterpret-cast to their
 *    private implementation structs.
 *
 * Example (in a port .c file):
 *
 *   typedef struct { ... } hal_uart_impl_t;
 *   _Static_assert(sizeof(hal_uart_impl_t) <= sizeof(((hal_uart_t*)0)->_opaque),
 *                  "hal_uart_t storage too small");
 *   static inline hal_uart_impl_t* U(hal_uart_t *u) { return (hal_uart_impl_t*)u->_opaque; }
 *
 * Rules:
 *  - No vendor SDK headers here (ESP-IDF, Pico SDK, STM32 HAL, etc.)
 *  - Keep this file stable: it gets included everywhere.
 */

#include <stdint.h>
#include <stddef.h>   // max_align_t

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------
 * Opaque handle storage sizing
 * ------------------------------------------------------------ */

/*
 * Sizes are expressed in BYTES.
 * Adjust these if a port's private impl grows larger.
 *
 * Recommendation:
 *  - Keep these modest but leave headroom.
 *  - Ports must _Static_assert() that their impl fits.
 */

#ifndef HAL_GPIO_HANDLE_BYTES
#define HAL_GPIO_HANDLE_BYTES   96
#endif

#ifndef HAL_UART_HANDLE_BYTES
#define HAL_UART_HANDLE_BYTES   32
#endif

#ifndef HAL_I2C_HANDLE_BYTES
#define HAL_I2C_HANDLE_BYTES    32
#endif

#ifndef HAL_SPI_HANDLE_BYTES
#define HAL_SPI_HANDLE_BYTES    64
#endif

#ifndef HAL_TIMER_HANDLE_BYTES
#define HAL_TIMER_HANDLE_BYTES  32
#endif

#ifndef HAL_ADC_HANDLE_BYTES
#define HAL_ADC_HANDLE_BYTES    32
#endif

#ifndef HAL_PWM_HANDLE_BYTES
#define HAL_PWM_HANDLE_BYTES    32
#endif

#ifndef HAL_I2S_HANDLE_BYTES
#define HAL_I2S_HANDLE_BYTES    96
#endif

#ifndef HAL_RMT_HANDLE_BYTES
#define HAL_RMT_HANDLE_BYTES    96
#endif

#ifndef HAL_DISPLAY_HANDLE_BYTES
#define HAL_DISPLAY_HANDLE_BYTES 64
#endif

#ifndef HAL_FILE_HANDLE_BYTES
#define HAL_FILE_HANDLE_BYTES   64
#endif

/*
 * Helper macro to define an opaque, stack-allocatable handle type.
 *
 * We use a union with max_align_t to guarantee alignment suitable for
 * reinterpret-casting to any private impl struct on the platform.
 *
 * The member MUST be named `_opaque` so ports can do:
 *   (impl_t*)handle->_opaque
 */
#define HAL_DEFINE_OPAQUE_HANDLE(type_name, bytes) \
    typedef union {                                \
        max_align_t _align;                        \
        uint8_t     _opaque[(bytes)];              \
    } type_name

/* ------------------------------------------------------------
 * HAL handle types
 * ------------------------------------------------------------ */

HAL_DEFINE_OPAQUE_HANDLE(hal_gpio_t,    HAL_GPIO_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_uart_t,    HAL_UART_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_i2c_t,     HAL_I2C_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_spi_t,     HAL_SPI_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_timer_t,   HAL_TIMER_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_adc_t,     HAL_ADC_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_pwm_t,     HAL_PWM_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_i2s_t,     HAL_I2S_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_rmt_t,     HAL_RMT_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_display_t, HAL_DISPLAY_HANDLE_BYTES);
HAL_DEFINE_OPAQUE_HANDLE(hal_file_t,    HAL_FILE_HANDLE_BYTES);

#undef HAL_DEFINE_OPAQUE_HANDLE

/* ------------------------------------------------------------
 * Common boolean / enable conventions
 * ------------------------------------------------------------ */

#ifndef HAL_TRUE
#define HAL_TRUE  1
#endif

#ifndef HAL_FALSE
#define HAL_FALSE 0
#endif

/* ------------------------------------------------------------
 * Common "active level" definition (CS pins, reset, etc.)
 * ------------------------------------------------------------ */

typedef enum {
    HAL_ACTIVE_LOW  = 0,
    HAL_ACTIVE_HIGH = 1,
} hal_active_t;

/* ------------------------------------------------------------
 * Common seek modes (filesystem)
 * ------------------------------------------------------------ */

typedef enum {
    HAL_SEEK_SET = 0,
    HAL_SEEK_CUR = 1,
    HAL_SEEK_END = 2,
} hal_seek_whence_t;

/* ------------------------------------------------------------
 * Common time types
 * ------------------------------------------------------------ */

typedef uint64_t hal_time_us_t;
typedef uint64_t hal_time_ms_t;

/* ------------------------------------------------------------
 * Generic IRQ callback type (used by GPIO, timers, etc.)
 * ------------------------------------------------------------ */

typedef void (*hal_irq_cb_t)(void *arg);

/* Timer callback is often the same shape, but keeping a distinct name helps readability. */
typedef void (*hal_timer_cb_t)(void *arg);

/* ------------------------------------------------------------
 * Optional lightweight stat structure (filesystem)
 * ------------------------------------------------------------ */

typedef struct {
    uint32_t size;      // bytes
    uint8_t  is_dir;    // 1 if directory, 0 if file
    uint8_t  reserved[3];
} hal_stat_t;

/* ------------------------------------------------------------
 * Common error model
 * ------------------------------------------------------------ */
/*
 * BasaltOS HAL uses POSIX-style negative errno values for failures.
 * Returns typically follow:
 *   0          success
 *   -errno     failure
 * or for read/write-like operations:
 *   bytes      success count
 *   -errno     failure
 *
 * We do not define errno values here to avoid conflicts; platforms
 * may include <errno.h> in .c files. Headers should document the
 * convention, not enforce a specific errno implementation.
 */

#ifdef __cplusplus
}
#endif
