#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_SPI_MODE0 = 0,
    HAL_SPI_MODE1 = 1,
    HAL_SPI_MODE2 = 2,
    HAL_SPI_MODE3 = 3,
} hal_spi_mode_t;

int hal_spi_init(hal_spi_t *spi,
                 int bus,
                 uint32_t freq_hz,
                 int sclk_pin,
                 int mosi_pin,
                 int miso_pin,
                 int cs_pin,
                 hal_spi_mode_t mode);

int hal_spi_deinit(hal_spi_t *spi);

int hal_spi_set_freq(hal_spi_t *spi, uint32_t freq_hz);

int hal_spi_set_mode(hal_spi_t *spi, hal_spi_mode_t mode);

int hal_spi_transfer(hal_spi_t *spi,
                     const uint8_t *tx,
                     uint8_t *rx,
                     size_t len,
                     uint32_t timeout_ms);

int hal_spi_write(hal_spi_t *spi,
                  const uint8_t *tx,
                  size_t len,
                  uint32_t timeout_ms);

int hal_spi_read(hal_spi_t *spi,
                 uint8_t *rx,
                 size_t len,
                 uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
