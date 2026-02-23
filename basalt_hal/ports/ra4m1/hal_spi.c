#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "hal/hal_spi.h"

typedef struct {
    int bus;
    uint32_t freq_hz;
    int sclk_pin;
    int mosi_pin;
    int miso_pin;
    int cs_pin;
    hal_spi_mode_t mode;
    int initialized;
} hal_spi_impl_t;

_Static_assert(sizeof(hal_spi_impl_t) <= sizeof(((hal_spi_t *)0)->_opaque),
               "hal_spi_t opaque storage too small for ra4m1 hal_spi_impl_t");

static inline hal_spi_impl_t *S(hal_spi_t *spi) {
    return (hal_spi_impl_t *)spi->_opaque;
}

int hal_spi_init(hal_spi_t *spi,
                 int bus,
                 uint32_t freq_hz,
                 int sclk_pin,
                 int mosi_pin,
                 int miso_pin,
                 int cs_pin,
                 hal_spi_mode_t mode) {
    if (!spi || bus < 0 || freq_hz == 0) return -EINVAL;
    if (sclk_pin < 0 || mosi_pin < 0 || miso_pin < 0 || cs_pin < 0) return -EINVAL;
    if (mode < HAL_SPI_MODE0 || mode > HAL_SPI_MODE3) return -EINVAL;
    hal_spi_impl_t *impl = S(spi);
    impl->bus = bus;
    impl->freq_hz = freq_hz;
    impl->sclk_pin = sclk_pin;
    impl->mosi_pin = mosi_pin;
    impl->miso_pin = miso_pin;
    impl->cs_pin = cs_pin;
    impl->mode = mode;
    impl->initialized = 1;
    return 0;
}

int hal_spi_deinit(hal_spi_t *spi) {
    if (!spi) return -EINVAL;
    hal_spi_impl_t *impl = S(spi);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_spi_set_freq(hal_spi_t *spi, uint32_t freq_hz) {
    if (!spi || freq_hz == 0) return -EINVAL;
    hal_spi_impl_t *impl = S(spi);
    if (!impl->initialized) return -EINVAL;
    impl->freq_hz = freq_hz;
    return 0;
}

int hal_spi_set_mode(hal_spi_t *spi, hal_spi_mode_t mode) {
    if (!spi) return -EINVAL;
    if (mode < HAL_SPI_MODE0 || mode > HAL_SPI_MODE3) return -EINVAL;
    hal_spi_impl_t *impl = S(spi);
    if (!impl->initialized) return -EINVAL;
    impl->mode = mode;
    return 0;
}

int hal_spi_transfer(hal_spi_t *spi,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len,
                     uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!spi) return -EINVAL;
    if (len > (size_t)INT_MAX) return -EMSGSIZE;
    hal_spi_impl_t *impl = S(spi);
    if (!impl->initialized) return -EINVAL;
    if (len == 0) return 0;
    if (!tx && !rx) return -EINVAL;
    if (rx && tx) {
        memcpy(rx, tx, len);
    } else if (rx) {
        memset(rx, 0xFF, len);
    }
    return (int)len;
}

int hal_spi_write(hal_spi_t *spi,
                  const uint8_t *tx,
                  size_t len,
                  uint32_t timeout_ms) {
    return hal_spi_transfer(spi, tx, NULL, len, timeout_ms);
}

int hal_spi_read(hal_spi_t *spi,
                 uint8_t *rx,
                 size_t len,
                 uint32_t timeout_ms) {
    return hal_spi_transfer(spi, NULL, rx, len, timeout_ms);
}
