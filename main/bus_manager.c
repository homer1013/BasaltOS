#include "bus_manager.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#ifdef __has_include
#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif
#endif

#if defined(SOC_SPI_PERIPH_NUM)
#define BASALT_SPI_HOST_MAX SOC_SPI_PERIPH_NUM
#elif defined(SPI_HOST_MAX)
#define BASALT_SPI_HOST_MAX SPI_HOST_MAX
#else
#define BASALT_SPI_HOST_MAX 4
#endif

typedef struct {
    bool configured;
    gpio_num_t sda;
    gpio_num_t scl;
    uint32_t hz;
    const char *owner;
} basalt_i2c_state_t;

typedef struct {
    bool configured;
    int dma_chan;
    int mosi;
    int miso;
    int sclk;
    int quadwp;
    int quadhd;
    int max_transfer_sz;
    const char *owner;
} basalt_spi_state_t;

static SemaphoreHandle_t s_bus_lock = NULL;
static basalt_i2c_state_t s_i2c_state[I2C_NUM_MAX];
static basalt_spi_state_t s_spi_state[BASALT_SPI_HOST_MAX];

static void basalt_bus_lock_init(void) {
    if (!s_bus_lock) {
        s_bus_lock = xSemaphoreCreateMutex();
    }
}

static void basalt_set_err(char *err, size_t err_len, const char *msg) {
    if (err && err_len) {
        snprintf(err, err_len, "%s", msg ? msg : "unknown error");
    }
}

static bool basalt_i2c_cfg_compatible(const basalt_i2c_state_t *st, const i2c_config_t *cfg) {
    return st->sda == cfg->sda_io_num &&
           st->scl == cfg->scl_io_num &&
           st->hz == cfg->master.clk_speed;
}

static bool basalt_spi_cfg_compatible(const basalt_spi_state_t *st,
                                      const spi_bus_config_t *cfg,
                                      int dma_chan) {
    if (st->dma_chan != dma_chan) {
        return false;
    }
    if (st->mosi != cfg->mosi_io_num ||
        st->miso != cfg->miso_io_num ||
        st->sclk != cfg->sclk_io_num ||
        st->quadwp != cfg->quadwp_io_num ||
        st->quadhd != cfg->quadhd_io_num) {
        return false;
    }
    if (cfg->max_transfer_sz > st->max_transfer_sz) {
        return false;
    }
    return true;
}

bool basalt_bus_i2c_master_ensure(i2c_port_t port,
                                  const i2c_config_t *cfg,
                                  const char *owner,
                                  char *err,
                                  size_t err_len) {
    if (!cfg) {
        basalt_set_err(err, err_len, "i2c cfg is null");
        return false;
    }
    if (cfg->mode != I2C_MODE_MASTER) {
        basalt_set_err(err, err_len, "i2c cfg must be master mode");
        return false;
    }
    if (port < 0 || port >= I2C_NUM_MAX) {
        basalt_set_err(err, err_len, "invalid i2c port");
        return false;
    }

    basalt_bus_lock_init();
    if (!s_bus_lock) {
        basalt_set_err(err, err_len, "bus lock init failed");
        return false;
    }
    xSemaphoreTake(s_bus_lock, portMAX_DELAY);

    basalt_i2c_state_t *st = &s_i2c_state[port];
    if (st->configured && !basalt_i2c_cfg_compatible(st, cfg)) {
        if (err && err_len) {
            snprintf(err, err_len,
                     "i2c port %d already owned by %s with different cfg",
                     (int)port, st->owner ? st->owner : "unknown");
        }
        xSemaphoreGive(s_bus_lock);
        return false;
    }

    esp_err_t ret = i2c_param_config(port, cfg);
    if (ret != ESP_OK) {
        if (err && err_len) {
            snprintf(err, err_len, "i2c_param_config failed (%s)", esp_err_to_name(ret));
        }
        xSemaphoreGive(s_bus_lock);
        return false;
    }
    ret = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) {
            snprintf(err, err_len, "i2c_driver_install failed (%s)", esp_err_to_name(ret));
        }
        xSemaphoreGive(s_bus_lock);
        return false;
    }

    st->configured = true;
    st->sda = cfg->sda_io_num;
    st->scl = cfg->scl_io_num;
    st->hz = cfg->master.clk_speed;
    st->owner = owner;
    xSemaphoreGive(s_bus_lock);
    return true;
}

bool basalt_bus_spi_ensure(spi_host_device_t host,
                           const spi_bus_config_t *cfg,
                           int dma_chan,
                           const char *owner,
                           char *err,
                           size_t err_len) {
    if (!cfg) {
        basalt_set_err(err, err_len, "spi cfg is null");
        return false;
    }
    if ((int)host < 0 || (int)host >= BASALT_SPI_HOST_MAX) {
        basalt_set_err(err, err_len, "invalid spi host");
        return false;
    }

    basalt_bus_lock_init();
    if (!s_bus_lock) {
        basalt_set_err(err, err_len, "bus lock init failed");
        return false;
    }
    xSemaphoreTake(s_bus_lock, portMAX_DELAY);

    basalt_spi_state_t *st = &s_spi_state[(int)host];
    if (st->configured && !basalt_spi_cfg_compatible(st, cfg, dma_chan)) {
        if (err && err_len) {
            snprintf(err, err_len,
                     "spi host %d already owned by %s with different cfg",
                     (int)host, st->owner ? st->owner : "unknown");
        }
        xSemaphoreGive(s_bus_lock);
        return false;
    }

    esp_err_t ret = spi_bus_initialize(host, cfg, dma_chan);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) {
            snprintf(err, err_len, "spi_bus_initialize failed (%s)", esp_err_to_name(ret));
        }
        xSemaphoreGive(s_bus_lock);
        return false;
    }

    st->configured = true;
    st->dma_chan = dma_chan;
    st->mosi = cfg->mosi_io_num;
    st->miso = cfg->miso_io_num;
    st->sclk = cfg->sclk_io_num;
    st->quadwp = cfg->quadwp_io_num;
    st->quadhd = cfg->quadhd_io_num;
    st->max_transfer_sz = cfg->max_transfer_sz;
    st->owner = owner;
    xSemaphoreGive(s_bus_lock);
    return true;
}
