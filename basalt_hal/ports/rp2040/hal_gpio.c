// BasaltOS RP2040 HAL - GPIO
//
// Minimal runtime contract implementation for:
//   hal/include/hal/hal_gpio.h
//
// Note:
// - This provides concrete function bodies and state handling for foundation
//   contract validation.
// - Hardware-specific register/LL binding can be layered in later slices.

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_gpio.h"

typedef struct {
    int pin;
    int initialized;
    int level;
    int irq_enabled;
    hal_gpio_mode_t mode;
    hal_gpio_pull_t pull;
    hal_gpio_drive_t drive;
    hal_gpio_irq_t irq_trig;
    hal_gpio_irq_cb_t irq_cb;
    void *irq_arg;
} hal_gpio_impl_t;

_Static_assert(sizeof(hal_gpio_impl_t) <= sizeof(((hal_gpio_t *)0)->_opaque),
               "hal_gpio_t opaque storage too small for rp2040 hal_gpio_impl_t");

static inline hal_gpio_impl_t *G(hal_gpio_t *g) {
    return (hal_gpio_impl_t *)g->_opaque;
}

static inline int gpio_valid_pin(int pin) {
    return pin >= 0;
}

int hal_gpio_init(hal_gpio_t *gpio, int pin) {
    if (!gpio || !gpio_valid_pin(pin)) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    g->pin = pin;
    g->initialized = 1;
    g->level = 0;
    g->irq_enabled = 0;
    g->mode = HAL_GPIO_INPUT;
    g->pull = HAL_GPIO_PULL_NONE;
    g->drive = HAL_GPIO_DRIVE_DEFAULT;
    g->irq_trig = HAL_GPIO_IRQ_NONE;
    g->irq_cb = NULL;
    g->irq_arg = NULL;
    return 0;
}

int hal_gpio_deinit(hal_gpio_t *gpio) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    g->initialized = 0;
    g->irq_enabled = 0;
    g->irq_cb = NULL;
    g->irq_arg = NULL;
    return 0;
}

int hal_gpio_set_mode(hal_gpio_t *gpio, hal_gpio_mode_t mode) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (mode < HAL_GPIO_INPUT || mode > HAL_GPIO_OPEN_DRAIN) return -EINVAL;
    g->mode = mode;
    return 0;
}

int hal_gpio_set_pull(hal_gpio_t *gpio, hal_gpio_pull_t pull) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (pull < HAL_GPIO_PULL_NONE || pull > HAL_GPIO_PULL_DOWN) return -EINVAL;
    g->pull = pull;
    return 0;
}

int hal_gpio_set_drive(hal_gpio_t *gpio, hal_gpio_drive_t drive) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (drive < HAL_GPIO_DRIVE_DEFAULT || drive > HAL_GPIO_DRIVE_HIGH) return -EINVAL;
    g->drive = drive;
    return 0;
}

int hal_gpio_read(hal_gpio_t *gpio, int *value) {
    if (!gpio || !value) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    *value = (g->level != 0) ? 1 : 0;
    return 0;
}

int hal_gpio_write(hal_gpio_t *gpio, int value) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (g->mode != HAL_GPIO_OUTPUT && g->mode != HAL_GPIO_OPEN_DRAIN) return -EPERM;
    g->level = (value != 0) ? 1 : 0;
    return 0;
}

int hal_gpio_toggle(hal_gpio_t *gpio) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (g->mode != HAL_GPIO_OUTPUT && g->mode != HAL_GPIO_OPEN_DRAIN) return -EPERM;
    g->level = g->level ? 0 : 1;
    return 0;
}

int hal_gpio_set_irq(hal_gpio_t *gpio,
                     hal_gpio_irq_t trig,
                     hal_gpio_irq_cb_t cb,
                     void *arg) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (trig < HAL_GPIO_IRQ_NONE || trig > HAL_GPIO_IRQ_HIGH) return -EINVAL;
    if (trig != HAL_GPIO_IRQ_NONE && !cb) return -EINVAL;

    g->irq_trig = trig;
    g->irq_cb = cb;
    g->irq_arg = arg;
    if (trig == HAL_GPIO_IRQ_NONE) {
        g->irq_enabled = 0;
    }
    return 0;
}

int hal_gpio_irq_enable(hal_gpio_t *gpio, int enable) {
    if (!gpio) return -EINVAL;
    hal_gpio_impl_t *g = G(gpio);
    if (!g->initialized) return -EINVAL;
    if (g->irq_trig == HAL_GPIO_IRQ_NONE || g->irq_cb == NULL) return -EINVAL;
    g->irq_enabled = (enable != 0) ? 1 : 0;
    return 0;
}

int hal_gpio_get_caps(int pin, uint32_t *caps) {
    if (!gpio_valid_pin(pin) || !caps) return -EINVAL;
    *caps = HAL_GPIO_CAP_INPUT |
            HAL_GPIO_CAP_OUTPUT |
            HAL_GPIO_CAP_OPEN_DRAIN |
            HAL_GPIO_CAP_PULL_UP |
            HAL_GPIO_CAP_PULL_DOWN |
            HAL_GPIO_CAP_IRQ;
    return 0;
}
