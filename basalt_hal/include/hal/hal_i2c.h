#pragma once

#include <stdint.h>
#include <stddef.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BasaltOS HAL - I2C
 *
 * Addressing:
 *  - All device addresses passed to these APIs are 7-bit addresses (0x00-0x7F).
 *    Do NOT left-shift the address and do NOT include the R/W bit.
 *
 * Return values:
 *  - Config / control calls: 0 on success, -errno on failure
 *  - Transfer calls: bytes transferred on success, -errno on failure
 *
 * timeout_ms:
 *  - 0 means "no wait" where supported (implementation-defined).
 *  - otherwise block up to timeout_ms.
 */

int hal_i2c_init(hal_i2c_t *i2c, int bus, uint32_t freq_hz, int sda_pin, int scl_pin);
int hal_i2c_deinit(hal_i2c_t *i2c);

int hal_i2c_set_freq(hal_i2c_t *i2c, uint32_t freq_hz);

/**
 * Probe for ACK from a device.
 * @return 0 if ACK, -ENODEV if NACK, -errno otherwise.
 */
int hal_i2c_probe(hal_i2c_t *i2c, uint8_t addr, uint32_t timeout_ms);

/** Write bytes to device. */
int hal_i2c_write(hal_i2c_t *i2c, uint8_t addr,
                  const uint8_t *data, size_t len,
                  uint32_t timeout_ms);

/** Read bytes from device. */
int hal_i2c_read(hal_i2c_t *i2c, uint8_t addr,
                 uint8_t *data, size_t len,
                 uint32_t timeout_ms);

/**
 * Common register pattern: write some bytes, then read back with repeated-start.
 * Typical: write 1-byte reg address, then read N bytes.
 */
int hal_i2c_write_read(hal_i2c_t *i2c, uint8_t addr,
                       const uint8_t *wdata, size_t wlen,
                       uint8_t *rdata, size_t rlen,
                       uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
