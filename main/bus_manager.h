#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "driver/spi_master.h"

bool basalt_bus_i2c_master_ensure(i2c_port_t port,
                                  const i2c_config_t *cfg,
                                  const char *owner,
                                  char *err,
                                  size_t err_len);

bool basalt_bus_spi_ensure(spi_host_device_t host,
                           const spi_bus_config_t *cfg,
                           int dma_chan,
                           const char *owner,
                           char *err,
                           size_t err_len);
