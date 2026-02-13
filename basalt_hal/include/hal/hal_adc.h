#pragma once

#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_ADC_ATTEN_DB_0 = 0,
    HAL_ADC_ATTEN_DB_2_5,
    HAL_ADC_ATTEN_DB_6,
    HAL_ADC_ATTEN_DB_11,
} hal_adc_atten_t;

int hal_adc_init(hal_adc_t *adc,
                 int unit,
                 int channel,
                 hal_adc_atten_t atten,
                 int width_bits);

int hal_adc_init_pin(hal_adc_t *adc,
                     int gpio_num,
                     hal_adc_atten_t atten,
                     int width_bits);

int hal_adc_deinit(hal_adc_t *adc);

int hal_adc_set_atten(hal_adc_t *adc, hal_adc_atten_t atten);

int hal_adc_set_width(hal_adc_t *adc, int width_bits);

int hal_adc_read_raw(hal_adc_t *adc, int *raw_out);

int hal_adc_read_mv(hal_adc_t *adc, int *mv_out);

#ifdef __cplusplus
}
#endif
