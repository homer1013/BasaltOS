#include "mpy_runtime.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_log.h"

static bool s_ready = false;
static uint8_t *s_heap = NULL;
static size_t s_heap_size = 64 * 1024;
static TaskHandle_t s_app_task = NULL;
static char s_app_path[128] = {0};
static char s_last_error[128] = {0};
static char s_last_result[128] = "never-run";
static const char *TAG = "mpy_runtime";

// MicroPython embed port API
#include "port/micropython_embed.h"
void basalt_module_init(void);
#include "esp_heap_caps.h"

static void set_last_error(const char *msg) {
    if (!msg || !msg[0]) {
        s_last_error[0] = '\0';
        return;
    }
    snprintf(s_last_error, sizeof(s_last_error), "%s", msg);
}

static void clear_last_error(void) {
    s_last_error[0] = '\0';
}

static void set_last_result(const char *msg) {
    if (!msg || !msg[0]) {
        snprintf(s_last_result, sizeof(s_last_result), "unknown");
        return;
    }
    snprintf(s_last_result, sizeof(s_last_result), "%s", msg);
}

static bool fail_with(char *err_buf, size_t err_len, const char *fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    set_last_error(msg);
    set_last_result("failed");
    if (err_buf && err_len) {
        snprintf(err_buf, err_len, "%s", msg);
    }
    ESP_LOGE(TAG, "%s", msg);
    return false;
}

void mpy_runtime_init(void) {
    if (s_ready) return;

    s_heap = heap_caps_malloc(s_heap_size, MALLOC_CAP_8BIT);
    if (!s_heap) {
        set_last_error("MicroPython heap allocation failed");
        s_ready = false;
        return;
    }

    volatile uint32_t stack_top = (uint32_t)esp_cpu_get_sp();
    mp_embed_init(s_heap, s_heap_size, (void *)stack_top);
    basalt_module_init();
    clear_last_error();
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

const char *mpy_runtime_last_error(void) {
    return s_last_error[0] ? s_last_error : NULL;
}

const char *mpy_runtime_last_result(void) {
    return s_last_result;
}

bool mpy_runtime_run_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        return fail_with(err_buf, err_len, "missing script path");
    }

    if (!s_ready) {
        mpy_runtime_init();
    }
    if (!s_ready) {
        return fail_with(err_buf, err_len, "MicroPython VM init failed");
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        return fail_with(err_buf, err_len, "cannot open %s", path);
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 64 * 1024) {
        fclose(f);
        return fail_with(err_buf, err_len, "invalid script size");
    }

    char *buf = malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return fail_with(err_buf, err_len, "out of memory");
    }
    size_t want = (size_t)sz;
    size_t n = fread(buf, 1, want, f);
    fclose(f);
    if (n != want) {
        free(buf);
        return fail_with(err_buf, err_len, "failed to read script (%u/%u bytes)",
                         (unsigned)n, (unsigned)want);
    }
    buf[n] = '\0';

    mp_embed_exec_str(buf);
    free(buf);
    clear_last_error();
    set_last_result("completed");
    return true;
}

static void mpy_app_task(void *arg) {
    char path[128];
    snprintf(path, sizeof(path), "%s", (const char *)arg);
    free(arg);

    char err[128];
    if (!mpy_runtime_run_file(path, err, sizeof(err))) {
        // Start succeeds before app task runs; surface runtime errors here.
        printf("run: %s\n", err);
        fflush(stdout);
    }

    s_app_path[0] = '\0';
    s_app_task = NULL;
    vTaskDelete(NULL);
}

bool mpy_runtime_start_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        return fail_with(err_buf, err_len, "missing script path");
    }
    if (s_app_task) {
        return fail_with(err_buf, err_len, "app already running");
    }
    char *path_copy = malloc(strlen(path) + 1);
    if (!path_copy) {
        return fail_with(err_buf, err_len, "out of memory");
    }
    strcpy(path_copy, path);
    snprintf(s_app_path, sizeof(s_app_path), "%s", path);
    if (xTaskCreate(mpy_app_task, "mpy_app", 8192, path_copy, 5, &s_app_task) != pdPASS) {
        free(path_copy);
        s_app_task = NULL;
        s_app_path[0] = '\0';
        return fail_with(err_buf, err_len, "failed to start app");
    }
    clear_last_error();
    set_last_result("running");
    return true;
}

bool mpy_runtime_stop(bool force, char *err_buf, size_t err_len) {
    if (!s_app_task) {
        return fail_with(err_buf, err_len, "no app running");
    }
    // No cooperative stop yet; force kill for now.
    (void)force;
    vTaskDelete(s_app_task);
    s_app_task = NULL;
    s_app_path[0] = '\0';
    mpy_runtime_deinit();
    clear_last_error();
    set_last_result(force ? "killed-by-user" : "stopped-by-user");
    return true;
}
