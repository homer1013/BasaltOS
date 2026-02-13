#pragma once

#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hal_pwm_init(hal_pwm_t *pwm,
                 int channel,
                 int gpio_pin,
                 uint32_t freq_hz,
                 int duty_resolution_bits);

int hal_pwm_deinit(hal_pwm_t *pwm);

int hal_pwm_set_duty_percent(hal_pwm_t *pwm, float duty_percent);

int hal_pwm_set_freq(hal_pwm_t *pwm, uint32_t freq_hz);

int hal_pwm_start(hal_pwm_t *pwm);

int hal_pwm_stop(hal_pwm_t *pwm);

#ifdef __cplusplus
}
#endif
