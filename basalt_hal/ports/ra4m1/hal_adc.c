#include <errno.h>
#include <stdint.h>

#include "hal/hal_adc.h"

typedef struct {
    int unit;
    int channel;
    int gpio_pin;
    int initialized;
    hal_adc_atten_t atten;
    int width_bits;
    uint32_t sample_counter;
} hal_adc_impl_t;

_Static_assert(sizeof(hal_adc_impl_t) <= sizeof(((hal_adc_t *)0)->_opaque),
               "hal_adc_t opaque storage too small for ra4m1 hal_adc_impl_t");

static inline hal_adc_impl_t *A(hal_adc_t *adc) {
    return (hal_adc_impl_t *)adc->_opaque;
}

static inline int adc_valid_width(int width_bits) {
    return width_bits >= 9 && width_bits <= 16;
}

static inline int adc_max_raw(const hal_adc_impl_t *impl) {
    return (1 << impl->width_bits) - 1;
}

int hal_adc_init(hal_adc_t *adc,
                 int unit,
                 int channel,
                 hal_adc_atten_t atten,
                 int width_bits) {
    if (!adc || unit < 0 || channel < 0) return -EINVAL;
    if (atten < HAL_ADC_ATTEN_DB_0 || atten > HAL_ADC_ATTEN_DB_11) return -EINVAL;
    if (!adc_valid_width(width_bits)) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    impl->unit = unit;
    impl->channel = channel;
    impl->gpio_pin = -1;
    impl->atten = atten;
    impl->width_bits = width_bits;
    impl->sample_counter = 0;
    impl->initialized = 1;
    return 0;
}

int hal_adc_init_pin(hal_adc_t *adc,
                     int gpio_num,
                     hal_adc_atten_t atten,
                     int width_bits) {
    int rc = hal_adc_init(adc, 0, gpio_num, atten, width_bits);
    if (rc != 0) return rc;
    A(adc)->gpio_pin = gpio_num;
    return 0;
}

int hal_adc_deinit(hal_adc_t *adc) {
    if (!adc) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_adc_set_atten(hal_adc_t *adc, hal_adc_atten_t atten) {
    if (!adc) return -EINVAL;
    if (atten < HAL_ADC_ATTEN_DB_0 || atten > HAL_ADC_ATTEN_DB_11) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    if (!impl->initialized) return -EINVAL;
    impl->atten = atten;
    return 0;
}

int hal_adc_set_width(hal_adc_t *adc, int width_bits) {
    if (!adc || !adc_valid_width(width_bits)) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    if (!impl->initialized) return -EINVAL;
    impl->width_bits = width_bits;
    return 0;
}

int hal_adc_read_raw(hal_adc_t *adc, int *raw_out) {
    if (!adc || !raw_out) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    if (!impl->initialized) return -EINVAL;
    impl->sample_counter++;
    *raw_out = (int)((impl->sample_counter * 97u) % (uint32_t)(adc_max_raw(impl) + 1));
    return 0;
}

int hal_adc_read_mv(hal_adc_t *adc, int *mv_out) {
    static const int full_scale_mv[] = {950, 1250, 1750, 2500};
    int raw = 0;
    if (!adc || !mv_out) return -EINVAL;
    hal_adc_impl_t *impl = A(adc);
    if (!impl->initialized) return -EINVAL;
    if (hal_adc_read_raw(adc, &raw) != 0) return -EINVAL;
    *mv_out = (raw * full_scale_mv[impl->atten]) / adc_max_raw(impl);
    return 0;
}
