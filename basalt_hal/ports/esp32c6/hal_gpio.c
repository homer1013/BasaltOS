// BasaltOS ESP32C6 HAL - GPIO
//
// ESP-IDF implementation of:
//   hal/include/hal/hal_gpio.h

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_gpio.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "hal_errno.h"
#include "esp_attr.h"

// -----------------------------------------------------------------------------
// Private implementation type stored inside hal_gpio_t opaque storage
// -----------------------------------------------------------------------------

typedef struct {
    int pin;
    bool initialized;

    hal_gpio_mode_t  mode;
    hal_gpio_pull_t  pull;
    hal_gpio_drive_t drive;

    // IRQ config
    hal_gpio_irq_t      irq_trig;
    hal_gpio_irq_cb_t   irq_cb;
    void              * irq_arg;
    bool               irq_configured;
} hal_gpio_impl_t;

_Static_assert(sizeof(hal_gpio_impl_t) <= sizeof(((hal_gpio_t *)0)->_opaque),
               "hal_gpio_t opaque storage too small for esp32 hal_gpio_impl_t");

static inline hal_gpio_impl_t *G(hal_gpio_t *g) {
    return (hal_gpio_impl_t *)g->_opaque;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static inline bool gpio_valid(int pin) {
    return GPIO_IS_VALID_GPIO((gpio_num_t)pin);
}

static gpio_mode_t map_mode(hal_gpio_mode_t mode) {
    switch (mode) {
        case HAL_GPIO_INPUT:
            return GPIO_MODE_INPUT;
        case HAL_GPIO_OUTPUT:
            return GPIO_MODE_OUTPUT;
        case HAL_GPIO_OPEN_DRAIN:
            // Open drain output; if you want input+output-od, change to GPIO_MODE_INPUT_OUTPUT_OD
            return GPIO_MODE_OUTPUT_OD;
        default:
            return GPIO_MODE_DISABLE;
    }
}

static int apply_config(hal_gpio_impl_t *g) {
    gpio_config_t cfg = {0};

    cfg.pin_bit_mask = (1ULL << (uint32_t)g->pin);
    cfg.intr_type = GPIO_INTR_DISABLE;
    cfg.mode = map_mode(g->mode);

    // Pull config
    switch (g->pull) {
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

    int rc = hal_esp_err_to_errno(gpio_config(&cfg));
    if (rc != 0) return rc;

    // Drive strength (only meaningful for output-capable pins)
    gpio_drive_cap_t cap = GPIO_DRIVE_CAP_2; // reasonable default
    switch (g->drive) {
        case HAL_GPIO_DRIVE_DEFAULT: cap = GPIO_DRIVE_CAP_2; break;
        case HAL_GPIO_DRIVE_LOW:     cap = GPIO_DRIVE_CAP_0; break;
        case HAL_GPIO_DRIVE_MEDIUM:  cap = GPIO_DRIVE_CAP_2; break;
        case HAL_GPIO_DRIVE_HIGH:    cap = GPIO_DRIVE_CAP_3; break;
        default: return -EINVAL;
    }

    // Ignore errors for input-only pins; gpio_set_drive_capability may fail there.
    (void)gpio_set_drive_capability((gpio_num_t)g->pin, cap);

    return 0;
}

static gpio_int_type_t map_irq_type(hal_gpio_irq_t trig) {
    switch (trig) {
        case HAL_GPIO_IRQ_NONE:    return GPIO_INTR_DISABLE;
        case HAL_GPIO_IRQ_RISING:  return GPIO_INTR_POSEDGE;
        case HAL_GPIO_IRQ_FALLING: return GPIO_INTR_NEGEDGE;
        case HAL_GPIO_IRQ_BOTH:    return GPIO_INTR_ANYEDGE;
        case HAL_GPIO_IRQ_LOW:     return GPIO_INTR_LOW_LEVEL;
        case HAL_GPIO_IRQ_HIGH:    return GPIO_INTR_HIGH_LEVEL;
        default:                   return GPIO_INTR_DISABLE;
    }
}

// ESP-IDF ISR service is global; install once.
static bool s_isr_service_installed = false;

static void IRAM_ATTR gpio_isr_thunk(void *arg) {
    hal_gpio_impl_t *g = (hal_gpio_impl_t *)arg;
    if (g && g->irq_cb) {
        g->irq_cb(g->irq_arg);
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

int hal_gpio_init(hal_gpio_t *gpio, int pin) {
    if (!gpio || !gpio_valid(pin)) return -EINVAL;

    hal_gpio_impl_t *g = G(gpio);

    g->pin = pin;
    g->initialized = true;

    // Defaults
    g->mode = HAL_GPIO_INPUT;
    g->pull = HAL_GPIO_PULL_NONE;
    g->drive = HAL_GPIO_DRIVE_DEFAULT;

    g->irq_trig = HAL_GPIO_IRQ_NONE;
    g->irq_cb = NULL;
    g->irq_arg = NULL;
    g->irq_configured = false;

    return apply_config(g);
}

int hal_gpio_deinit(hal_gpio_t *gpio) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    // Remove ISR handler if configured
    if (g->irq_configured) {
        (void)gpio_isr_handler_remove((gpio_num_t)g->pin);
        (void)gpio_set_intr_type((gpio_num_t)g->pin, GPIO_INTR_DISABLE);
        g->irq_configured = false;
        g->irq_cb = NULL;
        g->irq_arg = NULL;
        g->irq_trig = HAL_GPIO_IRQ_NONE;
    }

    esp_err_t e = gpio_reset_pin((gpio_num_t)g->pin);
    g->initialized = false;
    return hal_esp_err_to_errno(e);
}

int hal_gpio_set_mode(hal_gpio_t *gpio, hal_gpio_mode_t mode) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    // Basic validation: output/open-drain requires output-capable pins
    if ((mode == HAL_GPIO_OUTPUT || mode == HAL_GPIO_OPEN_DRAIN) &&
        !GPIO_IS_VALID_OUTPUT_GPIO((gpio_num_t)g->pin)) {
        return -ENOTSUP;
    }

    g->mode = mode;
    return apply_config(g);
}

int hal_gpio_set_pull(hal_gpio_t *gpio, hal_gpio_pull_t pull) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    g->pull = pull;
    return apply_config(g);
}

int hal_gpio_set_drive(hal_gpio_t *gpio, hal_gpio_drive_t drive) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    g->drive = drive;
    return apply_config(g);
}

int hal_gpio_read(hal_gpio_t *gpio, int *value) {
    if (!gpio || !value) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    *value = gpio_get_level((gpio_num_t)g->pin) ? 1 : 0;
    return 0;
}

int hal_gpio_write(hal_gpio_t *gpio, int value) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    // For safety, require output/open-drain mode to write
    if (g->mode == HAL_GPIO_INPUT) return -EPERM;

    return hal_esp_err_to_errno(gpio_set_level((gpio_num_t)g->pin, value ? 1 : 0));
}

int hal_gpio_toggle(hal_gpio_t *gpio) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    if (g->mode == HAL_GPIO_INPUT) return -EPERM;

    int lvl = gpio_get_level((gpio_num_t)g->pin);
    return hal_esp_err_to_errno(gpio_set_level((gpio_num_t)g->pin, lvl ? 0 : 1));
}

int hal_gpio_set_irq(hal_gpio_t *gpio,
                     hal_gpio_irq_t trig,
                     hal_gpio_irq_cb_t cb,
                     void *arg) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    // Install ISR service once
    if (!s_isr_service_installed) {
        esp_err_t ie = gpio_install_isr_service(0);
        if (ie != ESP_OK && ie != ESP_ERR_INVALID_STATE) {
            return hal_esp_err_to_errno(ie);
        }
        s_isr_service_installed = true;
    }

    // Remove existing handler if present
    if (g->irq_configured) {
        (void)gpio_isr_handler_remove((gpio_num_t)g->pin);
        g->irq_configured = false;
    }

    g->irq_trig = trig;
    g->irq_cb = cb;
    g->irq_arg = arg;

    // Disable if none
    if (trig == HAL_GPIO_IRQ_NONE || cb == NULL) {
        (void)gpio_set_intr_type((gpio_num_t)g->pin, GPIO_INTR_DISABLE);
        return 0;
    }

    // Configure trigger
    esp_err_t e1 = gpio_set_intr_type((gpio_num_t)g->pin, map_irq_type(trig));
    if (e1 != ESP_OK) return hal_esp_err_to_errno(e1);

    // Add handler (passes impl pointer)
    esp_err_t e2 = gpio_isr_handler_add((gpio_num_t)g->pin, gpio_isr_thunk, (void *)g);
    if (e2 != ESP_OK) return hal_esp_err_to_errno(e2);

    g->irq_configured = true;

    // Default to disabled until explicitly enabled (matches your API design)
    (void)gpio_intr_disable((gpio_num_t)g->pin);

    return 0;
}

int hal_gpio_irq_enable(hal_gpio_t *gpio, int enable) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;

    if (!g->irq_configured || g->irq_trig == HAL_GPIO_IRQ_NONE || g->irq_cb == NULL) {
        return -EINVAL;
    }

    esp_err_t e = enable ? gpio_intr_enable((gpio_num_t)g->pin)
                         : gpio_intr_disable((gpio_num_t)g->pin);
    return hal_esp_err_to_errno(e);
}

int hal_gpio_get_caps(int pin, uint32_t *caps) {
    if (!caps) return -EINVAL;
    if (!gpio_valid(pin)) return -EINVAL;

    uint32_t c = 0;

    // Basic capabilities
    c |= HAL_GPIO_CAP_INPUT;
    c |= HAL_GPIO_CAP_PULL_UP;
    c |= HAL_GPIO_CAP_PULL_DOWN;
    c |= HAL_GPIO_CAP_IRQ;

    if (GPIO_IS_VALID_OUTPUT_GPIO((gpio_num_t)pin)) {
        c |= HAL_GPIO_CAP_OUTPUT;
        c |= HAL_GPIO_CAP_OPEN_DRAIN;

        // PWM via LEDC is generally possible on output-capable pins (platform limits apply)
        c |= HAL_GPIO_CAP_PWM;
    } else {
        c |= HAL_GPIO_CAP_INPUT_ONLY;
    }

    // ESP32 strapping pins (classic ESP32). This may vary slightly by variant.
    // Marking them helps users avoid accidental boot mode issues.
    switch (pin) {
        case 0: case 2: case 4: case 5: case 12: case 15:
            c |= HAL_GPIO_CAP_BOOT_STRAP;
            break;
        default:
            break;
    }

    *caps = c;
    return 0;
}
