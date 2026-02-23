// BasaltOS ESP32-c3 HAL - ADC

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_adc.h"

#include "esp_err.h"
#include "hal_errno.h"
#include "esp_adc/adc_oneshot.h"

typedef struct {
    adc_oneshot_unit_handle_t unit;
    adc_unit_t unit_id;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    bool initialized;
} hal_adc_impl_t;

_Static_assert(sizeof(hal_adc_impl_t) <= sizeof(((hal_adc_t *)0)->_opaque),
               "hal_adc_t opaque storage too small for esp32c3 hal_adc_impl_t");

static inline hal_adc_impl_t *A(hal_adc_t *adc) {
    return (hal_adc_impl_t *)adc->_opaque;
}

static inline adc_atten_t map_atten(hal_adc_atten_t atten) {
    switch (atten) {
        case HAL_ADC_ATTEN_DB_0: return ADC_ATTEN_DB_0;
        case HAL_ADC_ATTEN_DB_2_5: return ADC_ATTEN_DB_2_5;
        case HAL_ADC_ATTEN_DB_6: return ADC_ATTEN_DB_6;
        case HAL_ADC_ATTEN_DB_11: return ADC_ATTEN_DB_12;
        default: return ADC_ATTEN_DB_12;
    }
}

static inline adc_bitwidth_t map_width(int bits) {
    if (bits <= 9) return ADC_BITWIDTH_9;
    if (bits == 10) return ADC_BITWIDTH_10;
    if (bits == 11) return ADC_BITWIDTH_11;
    return ADC_BITWIDTH_12;
}

static int apply_channel_cfg(hal_adc_impl_t *a) {
    adc_oneshot_chan_cfg_t cfg = {
        .atten = a->atten,
        .bitwidth = a->bitwidth,
    };
    esp_err_t e = adc_oneshot_config_channel(a->unit, a->channel, &cfg);
    return hal_esp_err_to_errno(e);
}

int hal_adc_init(hal_adc_t *adc,
                 int unit,
                 int channel,
                 hal_adc_atten_t atten,
                 int width_bits) {
    if (!adc) return -EINVAL;
    if (unit != 1) return -ENOTSUP;

    hal_adc_impl_t *a = A(adc);
    a->unit_id = ADC_UNIT_1;
    a->channel = (adc_channel_t)channel;
    a->atten = map_atten(atten);
    a->bitwidth = map_width(width_bits);
    a->unit = NULL;
    a->initialized = false;

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = a->unit_id,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t e = adc_oneshot_new_unit(&init_cfg, &a->unit);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    int rc = apply_channel_cfg(a);
    if (rc != 0) {
        (void)adc_oneshot_del_unit(a->unit);
        a->unit = NULL;
        return rc;
    }

    a->initialized = true;
    return 0;
}

int hal_adc_deinit(hal_adc_t *adc) {
    if (!adc) return -EINVAL;
    hal_adc_impl_t *a = A(adc);
    if (!a->initialized || !a->unit) return -EINVAL;

    esp_err_t e = adc_oneshot_del_unit(a->unit);
    a->unit = NULL;
    a->initialized = false;
    return hal_esp_err_to_errno(e);
}

int hal_adc_set_atten(hal_adc_t *adc, hal_adc_atten_t atten) {
    if (!adc) return -EINVAL;
    hal_adc_impl_t *a = A(adc);
    if (!a->initialized || !a->unit) return -EINVAL;

    a->atten = map_atten(atten);
    return apply_channel_cfg(a);
}

int hal_adc_set_width(hal_adc_t *adc, int width_bits) {
    if (!adc) return -EINVAL;
    hal_adc_impl_t *a = A(adc);
    if (!a->initialized || !a->unit) return -EINVAL;

    a->bitwidth = map_width(width_bits);
    return apply_channel_cfg(a);
}

int hal_adc_read_raw(hal_adc_t *adc, int *raw_out) {
    if (!adc || !raw_out) return -EINVAL;
    hal_adc_impl_t *a = A(adc);
    if (!a->initialized || !a->unit) return -EINVAL;

    int raw = 0;
    esp_err_t e = adc_oneshot_read(a->unit, a->channel, &raw);
    if (e != ESP_OK) return hal_esp_err_to_errno(e);

    *raw_out = raw;
    return 0;
}

int hal_adc_read_mv(hal_adc_t *adc, int *mv_out) {
    if (!adc || !mv_out) return -EINVAL;
    int raw = 0;
    int rc = hal_adc_read_raw(adc, &raw);
    if (rc != 0) return rc;

    // Conservative linear approximation for 12-bit ADC.
    *mv_out = (raw * 3300) / 4095;
    return 0;
}
