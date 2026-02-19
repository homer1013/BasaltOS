#include "basalt_lua_bindings.h"

#include <errno.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/hal_gpio.h"

#include "esp_log.h"

static const char *TAG = "basalt_lua_bind";

static bool s_ready = false;
static char s_summary[96] = "uninitialized";

static int binding_system_log(const char *msg) {
    ESP_LOGI(TAG, "%s", msg ? msg : "");
    return 0;
}

static int binding_gpio_mode(int pin, int mode) {
    hal_gpio_t gpio;
    int rc = hal_gpio_init(&gpio, pin);
    if (rc < 0) {
        return rc;
    }

    hal_gpio_mode_t hal_mode = HAL_GPIO_INPUT;
    switch (mode) {
        case 0: hal_mode = HAL_GPIO_INPUT; break;
        case 1: hal_mode = HAL_GPIO_OUTPUT; break;
        case 2: hal_mode = HAL_GPIO_OPEN_DRAIN; break;
        default:
            hal_gpio_deinit(&gpio);
            return -EINVAL;
    }

    rc = hal_gpio_set_mode(&gpio, hal_mode);
    int drc = hal_gpio_deinit(&gpio);
    return (rc < 0) ? rc : drc;
}

static int binding_gpio_write(int pin, int value) {
    hal_gpio_t gpio;
    int rc = hal_gpio_init(&gpio, pin);
    if (rc < 0) {
        return rc;
    }
    rc = hal_gpio_set_mode(&gpio, HAL_GPIO_OUTPUT);
    if (rc < 0) {
        hal_gpio_deinit(&gpio);
        return rc;
    }
    rc = hal_gpio_write(&gpio, value ? 1 : 0);
    int drc = hal_gpio_deinit(&gpio);
    return (rc < 0) ? rc : drc;
}

static int binding_gpio_read(int pin, int *value) {
    if (!value) {
        return -EINVAL;
    }

    hal_gpio_t gpio;
    int rc = hal_gpio_init(&gpio, pin);
    if (rc < 0) {
        return rc;
    }
    rc = hal_gpio_set_mode(&gpio, HAL_GPIO_INPUT);
    if (rc < 0) {
        hal_gpio_deinit(&gpio);
        return rc;
    }
    rc = hal_gpio_read(&gpio, value);
    int drc = hal_gpio_deinit(&gpio);
    return (rc < 0) ? rc : drc;
}

static int binding_timer_sleep_ms(uint32_t ms) {
    if (ms == 0) {
        taskYIELD();
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}

static int binding_fs_write_text(const char *path, const char *text) {
    if (!path || !text) {
        return -EINVAL;
    }
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return -errno;
    }
    int rc = fputs(text, fp);
    int cerr = fclose(fp);
    if (rc < 0) {
        return -EIO;
    }
    if (cerr != 0) {
        return -EIO;
    }
    return 0;
}

static int binding_fs_read_text(const char *path, char *out, size_t out_len) {
    if (!path || !out || out_len == 0) {
        return -EINVAL;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -errno;
    }
    size_t n = fread(out, 1, out_len - 1, fp);
    out[n] = '\0';
    int ferr = ferror(fp);
    int cerr = fclose(fp);
    if (ferr != 0) {
        return -EIO;
    }
    if (cerr != 0) {
        return -EIO;
    }
    return 0;
}

static const basalt_lua_bindings_api_t s_api = {
    .system_log = binding_system_log,
    .gpio_mode = binding_gpio_mode,
    .gpio_write = binding_gpio_write,
    .gpio_read = binding_gpio_read,
    .timer_sleep_ms = binding_timer_sleep_ms,
    .fs_write_text = binding_fs_write_text,
    .fs_read_text = binding_fs_read_text,
};

void basalt_lua_bindings_init(void) {
    snprintf(s_summary, sizeof(s_summary), "system.log gpio.mode/write/read timer.sleep_ms fs.write_text/read_text");
    s_ready = true;
}

bool basalt_lua_bindings_ready(void) {
    return s_ready;
}

const basalt_lua_bindings_api_t *basalt_lua_bindings_get(void) {
    return s_ready ? &s_api : NULL;
}

const char *basalt_lua_bindings_summary(void) {
    return s_summary;
}
