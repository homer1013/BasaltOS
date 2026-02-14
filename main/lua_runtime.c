#include "lua_runtime.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(__has_include)
#if __has_include("lua_embed.h")
#include "lua_embed.h"
#define BASALT_HAS_LUA_EMBED 1
#else
#define BASALT_HAS_LUA_EMBED 0
#endif
#else
#define BASALT_HAS_LUA_EMBED 0
#endif

static bool s_lua_ready = false;
static bool s_lua_running = false;
static char s_lua_current_app[128] = {0};
static char s_lua_last_error[128] = {0};
static char s_lua_last_result[32] = "never-run";

static void lua_set_last_error(const char *msg) {
    if (!msg || !msg[0]) {
        s_lua_last_error[0] = '\0';
        return;
    }
    snprintf(s_lua_last_error, sizeof(s_lua_last_error), "%s", msg);
}

static void lua_set_last_result(const char *msg) {
    if (!msg || !msg[0]) {
        snprintf(s_lua_last_result, sizeof(s_lua_last_result), "unknown");
        return;
    }
    snprintf(s_lua_last_result, sizeof(s_lua_last_result), "%s", msg);
}

static bool lua_fail_with(char *err_buf, size_t err_len, const char *fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    lua_set_last_error(msg);
    lua_set_last_result("failed");
    if (err_buf && err_len) {
        snprintf(err_buf, err_len, "%s", msg);
    }
    return false;
}

void lua_runtime_init(void) {
    s_lua_running = false;
#if BASALT_HAS_LUA_EMBED
    lua_embed_init();
    s_lua_ready = lua_embed_is_ready();
    if (s_lua_ready) {
        lua_set_last_error(NULL);
        lua_set_last_result("ready");
    } else {
        lua_set_last_error("lua runtime bindings unavailable");
        lua_set_last_result("unavailable");
    }
#else
    s_lua_ready = false;
    lua_set_last_error("lua runtime component not built");
    lua_set_last_result("unavailable");
#endif
}

bool lua_runtime_is_ready(void) {
    return s_lua_ready;
}

bool lua_runtime_run_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        return lua_fail_with(err_buf, err_len, "missing script path");
    }
    if (!s_lua_ready) {
        lua_runtime_init();
    }
    if (!s_lua_ready) {
        return lua_fail_with(err_buf, err_len, "lua runtime is not ready");
    }
    return lua_fail_with(err_buf, err_len, "lua vm execution is not integrated in this build yet");
}

bool lua_runtime_start_file(const char *path, char *err_buf, size_t err_len) {
    if (!path || !path[0]) {
        return lua_fail_with(err_buf, err_len, "missing script path");
    }
    if (s_lua_running) {
        return lua_fail_with(err_buf, err_len, "app already running");
    }
    if (!s_lua_ready) {
        lua_runtime_init();
    }
    if (!s_lua_ready) {
        return lua_fail_with(err_buf, err_len, "lua runtime is not ready");
    }
    snprintf(s_lua_current_app, sizeof(s_lua_current_app), "%s", path);
    s_lua_running = false;
    return lua_fail_with(err_buf, err_len, "lua vm execution is not integrated in this build yet");
}

bool lua_runtime_stop(bool force, char *err_buf, size_t err_len) {
    (void)force;
    if (!s_lua_running) {
        return lua_fail_with(err_buf, err_len, "no app running");
    }
    s_lua_running = false;
    s_lua_current_app[0] = '\0';
    lua_set_last_error(NULL);
    lua_set_last_result(force ? "killed-by-user" : "stopped-by-user");
    return true;
}

bool lua_runtime_is_running(void) {
    return s_lua_running;
}

const char *lua_runtime_current_app(void) {
    return s_lua_running ? s_lua_current_app : NULL;
}

const char *lua_runtime_last_error(void) {
    return s_lua_last_error[0] ? s_lua_last_error : NULL;
}

const char *lua_runtime_last_result(void) {
    return s_lua_last_result;
}
