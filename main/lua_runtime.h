#pragma once

#include <stdbool.h>
#include <stddef.h>

void lua_runtime_init(void);
bool lua_runtime_is_ready(void);
bool lua_runtime_run_file(const char *path, char *err_buf, size_t err_len);
bool lua_runtime_start_file(const char *path, char *err_buf, size_t err_len);
bool lua_runtime_stop(bool force, char *err_buf, size_t err_len);
bool lua_runtime_is_running(void);
const char *lua_runtime_current_app(void);
const char *lua_runtime_last_error(void);
const char *lua_runtime_last_result(void);
