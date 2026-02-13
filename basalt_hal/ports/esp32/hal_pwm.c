// BasaltOS ESP32 HAL - PWM (LEDC)

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_pwm.h"

#include "driver/ledc.h"
#include "esp_err.h"

typedef struct {
    ledc_mode_t speed_mode;
    ledc_channel_t channel;
    ledc_timer_t timer;
    int gpio_pin;
    uint32_t freq_hz;
    ledc_timer_bit_t resolution;
    bool initialized;
    bool started;
} hal_pwm_impl_t;

_Static_assert(sizeof(hal_pwm_impl_t) <= sizeof(((hal_pwm_t *)0)->_opaque),
               "hal_pwm_t opaque storage too small for esp32 hal_pwm_impl_t");

static inline hal_pwm_impl_t *P(hal_pwm_t *pwm) {
    return (hal_pwm_impl_t *)pwm->_opaque;
}

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

static inline ledc_timer_bit_t map_resolution(int bits) {
    if (bits <= 8) return LEDC_TIMER_8_BIT;
    if (bits <= 9) return LEDC_TIMER_9_BIT;
    if (bits <= 10) return LEDC_TIMER_10_BIT;
    if (bits <= 11) return LEDC_TIMER_11_BIT;
    if (bits <= 12) return LEDC_TIMER_12_BIT;
    if (bits <= 13) return LEDC_TIMER_13_BIT;
    if (bits <= 14) return LEDC_TIMER_14_BIT;
    return LEDC_TIMER_15_BIT;
}

static inline uint32_t duty_max(ledc_timer_bit_t res) {
    return (1u << (uint32_t)res) - 1u;
}

int hal_pwm_init(hal_pwm_t *pwm,
                 int channel,
                 int gpio_pin,
                 uint32_t freq_hz,
                 int duty_resolution_bits) {
    if (!pwm || channel < 0 || gpio_pin < 0 || freq_hz == 0) return -EINVAL;

    hal_pwm_impl_t *p = P(pwm);
    p->speed_mode = LEDC_LOW_SPEED_MODE;
    p->channel = (ledc_channel_t)channel;
    p->timer = (ledc_timer_t)channel;
    p->gpio_pin = gpio_pin;
    p->freq_hz = freq_hz;
    p->resolution = map_resolution(duty_resolution_bits);
    p->initialized = false;
    p->started = false;

    ledc_timer_config_t timer_cfg = {
        .speed_mode = p->speed_mode,
        .duty_resolution = p->resolution,
        .timer_num = p->timer,
        .freq_hz = (uint32_t)freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t e = ledc_timer_config(&timer_cfg);
    if (e != ESP_OK) return esp_err_to_errno(e);

    ledc_channel_config_t chan_cfg = {
        .gpio_num = gpio_pin,
        .speed_mode = p->speed_mode,
        .channel = p->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = p->timer,
        .duty = 0,
        .hpoint = 0,
    };
    e = ledc_channel_config(&chan_cfg);
    if (e != ESP_OK) return esp_err_to_errno(e);

    p->initialized = true;
    return 0;
}

int hal_pwm_deinit(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *p = P(pwm);
    if (!p->initialized) return -EINVAL;

    esp_err_t e = ledc_stop(p->speed_mode, p->channel, 0);
    p->initialized = false;
    p->started = false;
    return esp_err_to_errno(e);
}

int hal_pwm_set_duty_percent(hal_pwm_t *pwm, float duty_percent) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *p = P(pwm);
    if (!p->initialized) return -EINVAL;

    if (duty_percent < 0.0f) duty_percent = 0.0f;
    if (duty_percent > 100.0f) duty_percent = 100.0f;

    uint32_t max = duty_max(p->resolution);
    uint32_t duty = (uint32_t)((duty_percent / 100.0f) * (float)max);

    esp_err_t e = ledc_set_duty(p->speed_mode, p->channel, duty);
    if (e != ESP_OK) return esp_err_to_errno(e);

    e = ledc_update_duty(p->speed_mode, p->channel);
    return esp_err_to_errno(e);
}

int hal_pwm_set_freq(hal_pwm_t *pwm, uint32_t freq_hz) {
    if (!pwm || freq_hz == 0) return -EINVAL;
    hal_pwm_impl_t *p = P(pwm);
    if (!p->initialized) return -EINVAL;

    uint32_t out = ledc_set_freq(p->speed_mode, p->timer, freq_hz);
    if (out == 0) return -EIO;

    p->freq_hz = out;
    return 0;
}

int hal_pwm_start(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *p = P(pwm);
    if (!p->initialized) return -EINVAL;
    p->started = true;
    return 0;
}

int hal_pwm_stop(hal_pwm_t *pwm) {
    if (!pwm) return -EINVAL;
    hal_pwm_impl_t *p = P(pwm);
    if (!p->initialized) return -EINVAL;

    esp_err_t e = ledc_stop(p->speed_mode, p->channel, 0);
    if (e != ESP_OK) return esp_err_to_errno(e);

    p->started = false;
    return 0;
}
