#include "runtime_dispatch.h"

#include <stdio.h>
#include <string.h>

#include "mpy_runtime.h"

static char s_dispatch_last_error[128] = {0};
static char s_dispatch_last_result[32] = "never-run";
static basalt_runtime_kind_t s_dispatch_last_runtime = BASALT_RUNTIME_PYTHON;

static void dispatch_set_error(const char *msg) {
    if (!msg || !msg[0]) {
        s_dispatch_last_error[0] = '\0';
        return;
    }
    snprintf(s_dispatch_last_error, sizeof(s_dispatch_last_error), "%s", msg);
}

static void dispatch_set_result(const char *msg) {
    if (!msg || !msg[0]) {
        snprintf(s_dispatch_last_result, sizeof(s_dispatch_last_result), "unknown");
        return;
    }
    snprintf(s_dispatch_last_result, sizeof(s_dispatch_last_result), "%s", msg);
}

basalt_runtime_kind_t runtime_kind_from_string(const char *name) {
    if (!name || !name[0]) return BASALT_RUNTIME_PYTHON;
    char norm[24];
    size_t n = strlen(name);
    if (n >= sizeof(norm)) n = sizeof(norm) - 1;
    for (size_t i = 0; i < n; ++i) {
        char c = name[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        norm[i] = c;
    }
    norm[n] = '\0';
    if (strcmp(norm, "python") == 0 || strcmp(norm, "micropython") == 0) return BASALT_RUNTIME_PYTHON;
    if (strcmp(norm, "lua") == 0) return BASALT_RUNTIME_LUA;
    return BASALT_RUNTIME_UNKNOWN;
}

const char *runtime_kind_name(basalt_runtime_kind_t kind) {
    switch (kind) {
        case BASALT_RUNTIME_PYTHON: return "python";
        case BASALT_RUNTIME_LUA: return "lua";
        default: return "unknown";
    }
}

bool runtime_dispatch_start_file(basalt_runtime_kind_t kind, const char *path, char *err_buf, size_t err_len) {
    s_dispatch_last_runtime = kind;
    if (kind == BASALT_RUNTIME_PYTHON) {
        bool ok = mpy_runtime_start_file(path, err_buf, err_len);
        if (!ok) {
            const char *e = mpy_runtime_last_error();
            dispatch_set_error(e ? e : "python runtime start failed");
            dispatch_set_result("failed");
            return false;
        }
        dispatch_set_error(NULL);
        dispatch_set_result("running");
        return true;
    }

    if (kind == BASALT_RUNTIME_LUA) {
        const char *msg = "lua runtime is not integrated in this build yet";
        if (err_buf && err_len) snprintf(err_buf, err_len, "%s", msg);
        dispatch_set_error(msg);
        dispatch_set_result("failed");
        return false;
    }

    {
        const char *msg = "unknown runtime requested";
        if (err_buf && err_len) snprintf(err_buf, err_len, "%s", msg);
        dispatch_set_error(msg);
        dispatch_set_result("failed");
        return false;
    }
}

bool runtime_dispatch_stop(bool force, char *err_buf, size_t err_len) {
    // Current implementation supports Python runtime task lifecycle.
    bool ok = mpy_runtime_stop(force, err_buf, err_len);
    if (!ok) {
        const char *e = mpy_runtime_last_error();
        dispatch_set_error(e ? e : "runtime stop failed");
        dispatch_set_result("failed");
        return false;
    }
    dispatch_set_error(NULL);
    dispatch_set_result(force ? "killed-by-user" : "stopped-by-user");
    return true;
}

bool runtime_dispatch_is_running(void) {
    return mpy_runtime_is_running();
}

bool runtime_dispatch_is_ready(void) {
    return mpy_runtime_is_ready();
}

const char *runtime_dispatch_current_app(void) {
    return mpy_runtime_current_app();
}

const char *runtime_dispatch_last_error(void) {
    const char *runtime_err = mpy_runtime_last_error();
    if (runtime_err && runtime_err[0]) return runtime_err;
    return s_dispatch_last_error[0] ? s_dispatch_last_error : NULL;
}

const char *runtime_dispatch_last_result(void) {
    const char *runtime_result = mpy_runtime_last_result();
    if (runtime_result && runtime_result[0] && strcmp(runtime_result, "never-run") != 0) return runtime_result;
    return s_dispatch_last_result;
}

const char *runtime_dispatch_last_runtime(void) {
    return runtime_kind_name(s_dispatch_last_runtime);
}
