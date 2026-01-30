// BasaltOS ESP32 HAL - GPIO
//
// ESP-IDF implementation of:
//   hal/include/hal/hal_gpio.h

#include <errno.h>
#include <stdbool.h>

#include "hal/hal_gpio.h"

#include "driver/gpio.h"
#include "esp_err.h"

// -----------------------------------------------------------------------------
// Private type definition (opaque to users)
// -----------------------------------------------------------------------------

struct hal_gpio {
    int pin;
    bool initialized;
};

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

static inline bool gpio_valid(int pin) {
    return GPIO_IS_VALID_GPIO((gpio_num_t)pin);
}

static int apply_config(int pin,
                        hal_gpio_dir_t dir,
                        hal_gpio_pull_t pull) {
    gpio_config_t cfg = {0};

    cfg.pin_bit_mask = (1ULL << pin);
    cfg.intr_type = GPIO_INTR_DISABLE;

    // Direction
    switch (dir) {
        case HAL_GPIO_DIR_INPUT:
            cfg.mode = GPIO_MODE_INPUT;
            break;
        case HAL_GPIO_DIR_OUTPUT:
            cfg.mode = GPIO_MODE_OUTPUT;
            break;
        default:
            return -EINVAL;
    }

    // Pull
    switch (pull) {
        case HAL_GPIO_PULL_NONE:
            cfg.pull_up_en = GPIO_PULLUP_DISABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case HAL_GPIO_PULL_UP:
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case HAL_GPIO_PULL_DOWN:
            cfg.pull_up_en = GPIO_PULLUP_DISABLE;
            cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            return -EINVAL;
    }

    return esp_err_to_errno(gpio_config(&cfg));
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

int hal_gpio_init(hal_gpio_t *gpio, int pin) {
    if (!gpio || !gpio_valid(pin)) return -EINVAL;

    gpio->pin = pin;
    gpio->initialized = true;

    // Default safe state: input, no pull
    return apply_config(pin, HAL_GPIO_DIR_INPUT, HAL_GPIO_PULL_NONE);
}

int hal_gpio_deinit(hal_gpio_t *gpio) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    esp_err_t err = gpio_reset_pin((gpio_num_t)gpio->pin);
    gpio->initialized = false;

    return esp_err_to_errno(err);
}

int hal_gpio_set_dir(hal_gpio_t *gpio, hal_gpio_dir_t dir) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    // Preserve pull configuration by re-reading is not supported by ESP-IDF,
    // so we reapply with no pull if caller wants more control.
    return apply_config(gpio->pin, dir, HAL_GPIO_PULL_NONE);
}

int hal_gpio_set_pull(hal_gpio_t *gpio, hal_gpio_pull_t pull) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    // Preserve direction: infer from current mode is not exposed,
    // so we assume input when pull is set.
    return apply_config(gpio->pin, HAL_GPIO_DIR_INPUT, pull);
}

int hal_gpio_write(hal_gpio_t *gpio, int value) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    return esp_err_to_errno(
        gpio_set_level((gpio_num_t)gpio->pin, value ? 1 : 0)
    );
}

int hal_gpio_read(hal_gpio_t *gpio) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    return gpio_get_level((gpio_num_t)gpio->pin) ? 1 : 0;
}

int hal_gpio_toggle(hal_gpio_t *gpio) {
    if (!gpio || !gpio->initialized) return -EINVAL;

    int lvl = gpio_get_level((gpio_num_t)gpio->pin);
    return esp_err_to_errno(
        gpio_set_level((gpio_num_t)gpio->pin, lvl ? 0 : 1)
    );
}
