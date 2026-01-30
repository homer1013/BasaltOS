#pragma once
/*
 * BasaltOS Hardware Abstraction Layer - Common Types
 *
 * This header provides shared types, forward declarations, and enums used
 * across multiple HAL domains.
 *
 * Rules:
 *  - No vendor SDK headers here (ESP-IDF, Pico SDK, STM32 HAL, etc.)
 *  - Keep this file small and stable: it gets included everywhere.
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------
 * Opaque handle forward declarations
 * ------------------------------------------------------------ */

typedef struct hal_gpio {
    int _reserved;
} hal_gpio_t;
typedef struct hal_uart   hal_uart_t;
typedef struct hal_spi    hal_spi_t;
typedef struct hal_i2c    hal_i2c_t;
typedef struct hal_timer  hal_timer_t;
typedef struct hal_adc    hal_adc_t;
typedef struct hal_pwm    hal_pwm_t;
typedef struct hal_display hal_display_t;
typedef struct hal_file   hal_file_t;

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
    HAL_ACTIVE_LOW = 0,
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
    uint32_t size;     // bytes
    uint8_t  is_dir;   // 1 if directory, 0 if file
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

/* ------------------------------------------------------------
 * Optional "not supported" helpers
 * ------------------------------------------------------------ */
/*
 * Some platforms won't support some features. In those cases, the
 * implementation should return -ENOSYS (or equivalent) from the .c file.
 * We intentionally do NOT define ENOSYS here; keep it in implementation.
 */

#ifdef __cplusplus
}
#endif
