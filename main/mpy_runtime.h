#pragma once

#include <stdbool.h>
#include <stddef.h>

void mpy_runtime_init(void);
bool mpy_runtime_is_ready(void);
bool mpy_runtime_run_file(const char *path, char *err_buf, size_t err_len);
bool mpy_runtime_start_file(const char *path, char *err_buf, size_t err_len);
bool mpy_runtime_stop(bool force, char *err_buf, size_t err_len);
bool mpy_runtime_is_running(void);
const char *mpy_runtime_current_app(void);
