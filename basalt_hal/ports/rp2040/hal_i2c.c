// BasaltOS RP2040 HAL - I2C
//
// Minimal runtime contract implementation for:
//   hal/include/hal/hal_i2c.h

#include <errno.h>
#include <stdint.h>

#include "hal/hal_i2c.h"

typedef struct {
    int bus;
    uint32_t freq_hz;
    int sda_pin;
    int scl_pin;
    int initialized;
} hal_i2c_impl_t;

_Static_assert(sizeof(hal_i2c_impl_t) <= sizeof(((hal_i2c_t *)0)->_opaque),
               "hal_i2c_t opaque storage too small for rp2040 hal_i2c_impl_t");

static inline hal_i2c_impl_t *I(hal_i2c_t *i2c) {
    return (hal_i2c_impl_t *)i2c->_opaque;
}

static inline int valid_addr7(uint8_t addr) {
    return addr <= 0x7F;
}

int hal_i2c_init(hal_i2c_t *i2c, int bus, uint32_t freq_hz, int sda_pin, int scl_pin) {
    if (!i2c) return -EINVAL;
    if (bus < 0 || freq_hz == 0 || sda_pin < 0 || scl_pin < 0) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    impl->bus = bus;
    impl->freq_hz = freq_hz;
    impl->sda_pin = sda_pin;
    impl->scl_pin = scl_pin;
    impl->initialized = 1;
    return 0;
}

int hal_i2c_deinit(hal_i2c_t *i2c) {
    if (!i2c) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    impl->initialized = 0;
    return 0;
}

int hal_i2c_set_freq(hal_i2c_t *i2c, uint32_t freq_hz) {
    if (!i2c || freq_hz == 0) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    impl->freq_hz = freq_hz;
    return 0;
}

int hal_i2c_probe(hal_i2c_t *i2c, uint8_t addr, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!i2c || !valid_addr7(addr)) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    return -ENOSYS;
}

int hal_i2c_write(hal_i2c_t *i2c, uint8_t addr,
                  const uint8_t *data, size_t len,
                  uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!i2c || !valid_addr7(addr)) return -EINVAL;
    if (len > 0 && data == NULL) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    if (len == 0) return 0;
    return -ENOSYS;
}

int hal_i2c_read(hal_i2c_t *i2c, uint8_t addr,
                 uint8_t *data, size_t len,
                 uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!i2c || !valid_addr7(addr)) return -EINVAL;
    if (len > 0 && data == NULL) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    if (len == 0) return 0;
    return -ENOSYS;
}

int hal_i2c_write_read(hal_i2c_t *i2c, uint8_t addr,
                       const uint8_t *wdata, size_t wlen,
                       uint8_t *rdata, size_t rlen,
                       uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!i2c || !valid_addr7(addr)) return -EINVAL;
    if (wlen > 0 && wdata == NULL) return -EINVAL;
    if (rlen > 0 && rdata == NULL) return -EINVAL;
    hal_i2c_impl_t *impl = I(i2c);
    if (!impl->initialized) return -EINVAL;
    if (wlen == 0 && rlen == 0) return 0;
    return -ENOSYS;
}
