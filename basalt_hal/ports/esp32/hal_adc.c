// BasaltOS ESP32 HAL - ADC (oneshot backend)

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal/hal_adc.h"

#include "esp_err.h"
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
               "hal_adc_t opaque storage too small for esp32 hal_adc_impl_t");

static inline hal_adc_impl_t *A(hal_adc_t *adc) {
    return (hal_adc_impl_t *)adc->_opaque;
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
    return esp_err_to_errno(adc_oneshot_config_channel(a->unit, a->channel, &cfg));
}

int hal_adc_init(hal_adc_t *adc,
                 int unit,
                 int channel,
                 hal_adc_atten_t atten,
                 int width_bits) {
    if (!adc) return -EINVAL;
    adc_unit_t unit_id;
    if (unit == 1 || unit == (int)ADC_UNIT_1) {
        unit_id = ADC_UNIT_1;
    } else {
        return -ENOTSUP;
    }

    hal_adc_impl_t *a = A(adc);
    a->unit_id = unit_id;
    a->channel = (adc_channel_t)channel;
    a->atten = map_atten(atten);
    a->bitwidth = map_width(width_bits);
    a->unit = NULL;
    a->initialized = false;

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = a->unit_id,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &a->unit);
    if (ret != ESP_OK) return esp_err_to_errno(ret);

    int rc = apply_channel_cfg(a);
    if (rc != 0) {
        (void)adc_oneshot_del_unit(a->unit);
        a->unit = NULL;
        return rc;
    }
    a->initialized = true;
    return 0;
}

int hal_adc_init_pin(hal_adc_t *adc,
                     int gpio_num,
                     hal_adc_atten_t atten,
                     int width_bits) {
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32)
    adc_unit_t unit = ADC_UNIT_1;
    adc_channel_t ch = ADC_CHANNEL_0;
    esp_err_t ret = adc_oneshot_io_to_channel(gpio_num, &unit, &ch);
    if (ret != ESP_OK) return esp_err_to_errno(ret);
    return hal_adc_init(adc, (int)unit, (int)ch, atten, width_bits);
#else
    (void)adc;
    (void)gpio_num;
    (void)atten;
    (void)width_bits;
    return -ENOSYS;
#endif
}

int hal_adc_deinit(hal_adc_t *adc) {
    if (!adc) return -EINVAL;
    hal_adc_impl_t *a = A(adc);
    if (!a->initialized || !a->unit) return -EINVAL;
    esp_err_t ret = adc_oneshot_del_unit(a->unit);
    a->unit = NULL;
    a->initialized = false;
    return esp_err_to_errno(ret);
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
    esp_err_t ret = adc_oneshot_read(a->unit, a->channel, &raw);
    if (ret != ESP_OK) return esp_err_to_errno(ret);
    *raw_out = raw;
    return 0;
}

int hal_adc_read_mv(hal_adc_t *adc, int *mv_out) {
    if (!adc || !mv_out) return -EINVAL;
    int raw = 0;
    int rc = hal_adc_read_raw(adc, &raw);
    if (rc != 0) return rc;
    *mv_out = (raw * 3300) / 4095;
    return 0;
}
