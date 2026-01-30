#pragma once
#include "hal_types.h"

typedef struct hal_gpio hal_gpio_t;

typedef enum {
    HAL_GPIO_INPUT,
    HAL_GPIO_OUTPUT,
    HAL_GPIO_OPEN_DRAIN,
} hal_gpio_mode_t;

typedef enum {
    HAL_GPIO_IRQ_NONE,
    HAL_GPIO_IRQ_RISING,
    HAL_GPIO_IRQ_FALLING,
    HAL_GPIO_IRQ_BOTH,
} hal_gpio_irq_t;

int hal_gpio_init(hal_gpio_t *gpio, int pin);
int hal_gpio_deinit(hal_gpio_t *gpio);

int hal_gpio_set_mode(hal_gpio_t *gpio, hal_gpio_mode_t mode);
int hal_gpio_set_pull(hal_gpio_t *gpio, hal_gpio_pull_t pull);

int hal_gpio_read(hal_gpio_t *gpio, int *value);
int hal_gpio_write(hal_gpio_t *gpio, int value);
int hal_gpio_toggle(hal_gpio_t *gpio);

int hal_gpio_set_irq(hal_gpio_t *gpio,
                     hal_gpio_irq_t trig,
                     hal_irq_cb_t cb,
                     void *arg);
