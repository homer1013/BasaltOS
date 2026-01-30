#pragma once
/*
 * BasaltOS Hardware Abstraction Layer - GPIO
 *
 * Portable GPIO contract used by BasaltOS.
 *
 * Rules:
 *  - No vendor SDK headers here (ESP-IDF, Pico SDK, STM32 HAL, etc.)
 *  - Error model:
 *      0 / -errno on failure (POSIX-style negative error codes)
 */

#include <stdint.h>
#include <stddef.h>
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------
 * GPIO modes
 * ------------------------------------------------------------ */

typedef enum {
    HAL_GPIO_INPUT = 0,
    HAL_GPIO_OUTPUT,
    HAL_GPIO_OPEN_DRAIN,
} hal_gpio_mode_t;

/* ------------------------------------------------------------
 * GPIO pull configuration
 * ------------------------------------------------------------ */

typedef enum {
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN,
} hal_gpio_pull_t;

/* ------------------------------------------------------------
 * GPIO drive strength (optional, platform-dependent)
 * ------------------------------------------------------------ */

typedef enum {
    HAL_GPIO_DRIVE_DEFAULT = 0,
    HAL_GPIO_DRIVE_LOW,
    HAL_GPIO_DRIVE_MEDIUM,
    HAL_GPIO_DRIVE_HIGH,
} hal_gpio_drive_t;

/* ------------------------------------------------------------
 * GPIO interrupt trigger types
 * ------------------------------------------------------------ */

typedef enum {
    HAL_GPIO_IRQ_NONE = 0,
    HAL_GPIO_IRQ_RISING,
    HAL_GPIO_IRQ_FALLING,
    HAL_GPIO_IRQ_BOTH,
    HAL_GPIO_IRQ_LOW,
    HAL_GPIO_IRQ_HIGH,
} hal_gpio_irq_t;

/* ------------------------------------------------------------
 * GPIO capability flags (for introspection)
 * ------------------------------------------------------------ */
/*
 * These are "pin capability" bits used for discovery / validation.
 * Example: input-only pins, boot strap pins, ADC-capable pins, etc.
 */
typedef enum {
    HAL_GPIO_CAP_INPUT        = (1u << 0),
    HAL_GPIO_CAP_OUTPUT       = (1u << 1),
    HAL_GPIO_CAP_OPEN_DRAIN   = (1u << 2),
    HAL_GPIO_CAP_PULL_UP      = (1u << 3),
    HAL_GPIO_CAP_PULL_DOWN    = (1u << 4),
    HAL_GPIO_CAP_IRQ          = (1u << 5),
    HAL_GPIO_CAP_PWM          = (1u << 6),
    HAL_GPIO_CAP_ADC          = (1u << 7),
    HAL_GPIO_CAP_BOOT_STRAP   = (1u << 8),
    HAL_GPIO_CAP_INPUT_ONLY   = (1u << 9),
} hal_gpio_cap_t;

/* ------------------------------------------------------------
 * IRQ callback type
 * ------------------------------------------------------------ */
/*
 * For consistency across HAL domains, we use hal_irq_cb_t from hal_types.h.
 * If you prefer a GPIO-specific alias, keep the typedef below; otherwise
 * you can remove it and use hal_irq_cb_t directly.
 */
typedef hal_irq_cb_t hal_gpio_irq_cb_t;

/* ------------------------------------------------------------
 * API
 * ------------------------------------------------------------ */

/**
 * @brief Initialize a GPIO object and bind it to a physical pin.
 *
 * @param gpio  GPIO object storage (provided by caller)
 * @param pin   Platform-specific pin number
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_init(hal_gpio_t *gpio, int pin);

/**
 * @brief Deinitialize a GPIO object and release the pin.
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_deinit(hal_gpio_t *gpio);

/**
 * @brief Set the GPIO mode (input, output, open-drain).
 *
 * Intended to be ISR-safe if the platform supports it.
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_set_mode(hal_gpio_t *gpio, hal_gpio_mode_t mode);

/**
 * @brief Configure pull-up / pull-down resistors.
 *
 * Platforms/pins that do not support pulls may ignore or return -ENOSYS.
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_set_pull(hal_gpio_t *gpio, hal_gpio_pull_t pull);

/**
 * @brief Configure drive strength (optional).
 *
 * Platforms that do not support drive strength should return -ENOSYS.
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_set_drive(hal_gpio_t *gpio, hal_gpio_drive_t drive);

/**
 * @brief Read the GPIO logic level.
 *
 * @param value Receives 0 or 1
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_read(hal_gpio_t *gpio, int *value);

/**
 * @brief Write the GPIO logic level.
 *
 * @param value 0 or 1
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_write(hal_gpio_t *gpio, int value);

/**
 * @brief Toggle the GPIO output.
 *
 * Optional convenience function (can be implemented via read+write or native toggle).
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_toggle(hal_gpio_t *gpio);

/**
 * @brief Configure a GPIO interrupt.
 *
 * @param trig Trigger condition
 * @param cb   Callback function
 * @param arg  User argument passed to callback
 *
 * Callback may execute in ISR context or deferred context, depending on platform.
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_set_irq(hal_gpio_t *gpio,
                     hal_gpio_irq_t trig,
                     hal_gpio_irq_cb_t cb,
                     void *arg);

/**
 * @brief Enable or disable a configured GPIO interrupt.
 *
 * @param enable 0=disable, nonzero=enable
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_irq_enable(hal_gpio_t *gpio, int enable);

/**
 * @brief Query pin capabilities.
 *
 * @param pin   Platform-specific pin number
 * @param caps  Receives ORed hal_gpio_cap_t flags
 *
 * @return 0 on success, -errno on failure
 */
int hal_gpio_get_caps(int pin, uint32_t *caps);

#ifdef __cplusplus
}
#endif
