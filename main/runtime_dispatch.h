#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BASALT_RUNTIME_PYTHON = 0,
    BASALT_RUNTIME_LUA = 1,
    BASALT_RUNTIME_UNKNOWN = 2,
} basalt_runtime_kind_t;

basalt_runtime_kind_t runtime_kind_from_string(const char *name);
const char *runtime_kind_name(basalt_runtime_kind_t kind);

bool runtime_dispatch_start_file(basalt_runtime_kind_t kind, const char *path, char *err_buf, size_t err_len);
bool runtime_dispatch_stop(bool force, char *err_buf, size_t err_len);
bool runtime_dispatch_is_running(void);
bool runtime_dispatch_is_ready(void);
const char *runtime_dispatch_current_app(void);
const char *runtime_dispatch_last_error(void);
const char *runtime_dispatch_last_result(void);
const char *runtime_dispatch_last_runtime(void);
