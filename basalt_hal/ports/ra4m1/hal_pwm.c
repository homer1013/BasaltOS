#include <errno.h>
#include <stdint.h>

#include "hal/hal_pwm.h"

typedef struct {
    int channel;
    int gpio_pin;
    uint32_t freq_hz;
    int duty_resolution_bits;
    int duty_permil;
    int initialized;
    int running;
} hal_pwm_impl_t;

_Static_assert(sizeof(hal_pwm_impl_t) <= sizeof(((hal_pwm_t *)0)->_opaque),
               "hal_pwm_t opaque storage too small for ra4m1 hal_pwm_impl_t");

static inline hal_pwm_impl_t *P(hal_pwm_t *pwm) {
    return (hal_pwm_impl_t *)pwm->_opaque;
}

int hal_pwm_init(hal_pwm_t *pwm,
                 int channel,
                 int gpio_pin,
                 uint32_t freq_hz,
                 int duty_resolution_bits) {
    if (!pwm || channel < 0 || gpio_pin < 0 || freq_hz == 0) return -EINVAL;
    if (duty_resolution_bits < 1 || duty_resolution_bits > 16) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    impl->channel = channel;
    impl->gpio_pin = gpio_pin;
    impl->freq_hz = freq_hz;
    impl->duty_resolution_bits = duty_resolution_bits;
    impl->duty_permil = 0;
    impl->initialized = 1;
    impl->running = 0;
    return 0;
}

int hal_pwm_deinit(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    impl->running = 0;
    return 0;
}

int hal_pwm_set_duty_percent(hal_pwm_t *pwm, float duty_percent) {
    if (!pwm) return -EINVAL;
    if (duty_percent < 0.0f || duty_percent > 100.0f) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    if (!impl->initialized) return -EINVAL;
    impl->duty_permil = (int)(duty_percent * 10.0f);
    return 0;
}

int hal_pwm_set_freq(hal_pwm_t *pwm, uint32_t freq_hz) {
    if (!pwm || freq_hz == 0) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    if (!impl->initialized) return -EINVAL;
    impl->freq_hz = freq_hz;
    return 0;
}

int hal_pwm_start(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    if (!impl->initialized) return -EINVAL;
    impl->running = 1;
    return 0;
}

int hal_pwm_stop(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *impl = P(pwm);
    if (!impl->initialized) return -EINVAL;
    impl->running = 0;
    return 0;
}
