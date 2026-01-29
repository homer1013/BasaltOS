#include "mpy_runtime.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_cpu.h"

static bool s_ready = false;
static uint8_t *s_heap = NULL;
static size_t s_heap_size = 64 * 1024;
static TaskHandle_t s_app_task = NULL;
static char s_app_path[128] = {0};

// MicroPython embed port API
#include "port/micropython_embed.h"
void basalt_module_init(void);
#include "esp_heap_caps.h"

void mpy_runtime_init(void) {
    if (s_ready) return;

    s_heap = heap_caps_malloc(s_heap_size, MALLOC_CAP_8BIT);
    if (!s_heap) {
        s_ready = false;
        return;
    }

    volatile uint32_t stack_top = (uint32_t)esp_cpu_get_sp();
    mp_embed_init(s_heap, s_heap_size, (void *)stack_top);
    basalt_module_init();
    s_ready = true;
}

static void mpy_runtime_deinit(void) {
    if (!s_ready) return;
    mp_embed_deinit();
    if (s_heap) {
        free(s_heap);
        s_heap = NULL;
    }
    s_ready = false;
}

bool mpy_runtime_is_ready(void) {
    return s_ready;
}

bool mpy_runtime_is_running(void) {
    return s_app_task != NULL;
}

const char *mpy_runtime_current_app(void) {
    return s_app_task ? s_app_path : NULL;
}

bool mpy_runtime_run_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "missing script path");
        }
        return false;
    }

    if (!s_ready) {
        mpy_runtime_init();
    }
    if (!s_ready) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "MicroPython VM init failed");
        }
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "cannot open %s", path);
        }
        return false;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 64 * 1024) {
        fclose(f);
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "invalid script size");
        }
        return false;
    }

    char *buf = malloc(sz + 1);
    if (!buf) {
        fclose(f);
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "out of memory");
        }
        return false;
    }
    size_t n = fread(buf, 1, sz, f);
    fclose(f);
    buf[n] = '\0';

    mp_embed_exec_str(buf);
    free(buf);
    return true;
}

static void mpy_app_task(void *arg) {
    char path[128];
    snprintf(path, sizeof(path), "%s", (const char *)arg);
    free(arg);

    char err[128];
    if (!mpy_runtime_run_file(path, err, sizeof(err))) {
        // Error is reported to caller when starting the app.
    }

    s_app_path[0] = '\0';
    s_app_task = NULL;
    vTaskDelete(NULL);
}

bool mpy_runtime_start_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "missing script path");
        }
        return false;
    }
    if (s_app_task) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "app already running");
        }
        return false;
    }
    char *path_copy = malloc(strlen(path) + 1);
    if (!path_copy) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "out of memory");
        }
        return false;
    }
    strcpy(path_copy, path);
    snprintf(s_app_path, sizeof(s_app_path), "%s", path);
    if (xTaskCreate(mpy_app_task, "mpy_app", 8192, path_copy, 5, &s_app_task) != pdPASS) {
        free(path_copy);
        s_app_task = NULL;
        s_app_path[0] = '\0';
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "failed to start app");
        }
        return false;
    }
    return true;
}

bool mpy_runtime_stop(bool force, char *err_buf, size_t err_len) {
    if (!s_app_task) {
        if (err_buf && err_len) {
            snprintf(err_buf, err_len, "no app running");
        }
        return false;
    }
    // No cooperative stop yet; force kill for now.
    (void)force;
    vTaskDelete(s_app_task);
    s_app_task = NULL;
    s_app_path[0] = '\0';
    mpy_runtime_deinit();
    return true;
}
