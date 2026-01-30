// BasaltOS/runtime/runtime_api.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------
// Versioning / feature flags
// -----------------------------
#define BASALT_RUNTIME_API_VERSION 1

typedef enum {
    RUNTIME_OK = 0,
    RUNTIME_EINVAL = -22,
    RUNTIME_ENOSYS = -38,
    RUNTIME_EIO = -5,
    RUNTIME_ENOMEM = -12,
    RUNTIME_ETIMEDOUT = -110,
} runtime_err_t;

typedef enum {
    RUNTIME_FEATURE_CONSOLE   = (1u << 0),
    RUNTIME_FEATURE_FS        = (1u << 1),
    RUNTIME_FEATURE_KV        = (1u << 2),
    RUNTIME_FEATURE_EVENTS    = (1u << 3),
    RUNTIME_FEATURE_TIMERS    = (1u << 4),
} runtime_feature_t;

typedef struct {
    uint32_t api_version;      // BASALT_RUNTIME_API_VERSION
    uint32_t features;         // bitset of runtime_feature_t
} runtime_info_t;

// -----------------------------
// Console / logging (engine-agnostic)
// -----------------------------
typedef enum {
    RT_CONSOLE_STDOUT = 0,
    RT_CONSOLE_STDERR = 1,
} rt_console_stream_t;

// Write raw bytes (no UTF-8 assumptions). Should be safe early in boot.
int rt_console_write(rt_console_stream_t stream, const void *buf, size_t len);

// Read bytes; returns bytes read, 0 on timeout, or -errno-like.
int rt_console_read(void *buf, size_t len, uint32_t timeout_ms);

// Optional: readline helper (can be implemented using rt_console_read internally).
// Returns bytes copied (excluding '\0'), 0 on timeout, or negative error.
int rt_console_readline(char *out, size_t out_len, uint32_t timeout_ms);

// Optional structured log (BasaltOS can route to UART, TFT console, file, etc.)
typedef enum {
    RT_LOG_DEBUG,
    RT_LOG_INFO,
    RT_LOG_WARN,
    RT_LOG_ERROR,
} rt_log_level_t;

int rt_log(rt_log_level_t level, const char *tag, const char *msg);

// -----------------------------
// Time / delays
// -----------------------------
uint64_t rt_time_us(void);              // monotonic time since boot
void     rt_sleep_ms(uint32_t ms);      // coarse sleep/yield (non-ISR)

// -----------------------------
// Filesystem (optional; designed to map to SPIFFS/FATFS/etc.)
// -----------------------------
typedef struct rt_file rt_file_t; // opaque handle

typedef enum {
    RT_F_RDONLY = 1u << 0,
    RT_F_WRONLY = 1u << 1,
    RT_F_RDWR   = 1u << 2,
    RT_F_CREAT  = 1u << 3,
    RT_F_TRUNC  = 1u << 4,
    RT_F_APPEND = 1u << 5,
} rt_fopen_flags_t;

int rt_fs_mount(const char *label, const char *path);   // e.g. "spiffs" -> "/fs"
int rt_fs_unmount(const char *path);

int rt_fs_open(rt_file_t **out, const char *path, uint32_t flags);
int rt_fs_close(rt_file_t *f);
int rt_fs_read(rt_file_t *f, void *buf, size_t len, uint32_t timeout_ms);   // bytes/-err
int rt_fs_write(rt_file_t *f, const void *buf, size_t len, uint32_t timeout_ms); // bytes/-err
int rt_fs_seek(rt_file_t *f, int32_t off, int whence);   // newpos/-err
int rt_fs_sync(rt_file_t *f);

int rt_fs_listdir(const char *path,
                  int (*cb)(const char *name, int is_dir, void *arg),
                  void *arg);

int rt_fs_remove(const char *path);
int rt_fs_rename(const char *old_path, const char *new_path);

// -----------------------------
// Key/Value (optional; maps to NVS, flash kv, etc.)
// -----------------------------
int rt_kv_init(void);
int rt_kv_get(const char *key, void *buf, size_t *len_inout);    // if buf==NULL -> returns required len in *len_inout
int rt_kv_set(const char *key, const void *buf, size_t len);
int rt_kv_del(const char *key);
int rt_kv_commit(void);

// -----------------------------
// Events (optional; lets runtimes integrate without owning the OS loop)
// -----------------------------
typedef enum {
    RT_EVT_NONE = 0,
    RT_EVT_KEY,         // keyboard/console keypress events (optional)
    RT_EVT_TIMER,       // periodic runtime timer callback tick
    RT_EVT_USER,        // generic user/system event
} rt_event_type_t;

typedef struct {
    rt_event_type_t type;
    uint32_t code;
    uint32_t a, b;
    void *ptr;
} rt_event_t;

// Post an event into the OS event queue (thread-safe if enabled).
int rt_event_post(const rt_event_t *evt);

// Poll next event; returns 1 if event, 0 if timeout, negative error.
int rt_event_poll(rt_event_t *evt, uint32_t timeout_ms);

// -----------------------------
// Runtime registration / command surface
// -----------------------------
// The idea: the runtime can expose commands/modules without hard-linking to UI.
// BasaltOS can list these in its shell / menu.

typedef int (*rt_command_fn)(int argc, char **argv);

int rt_register_command(const char *name,
                        const char *help,
                        rt_command_fn fn);

// Optional: allow a runtime to evaluate a line of script (for REPL / scripting).
// Return 0 on success, negative on error.
int rt_eval_line(const char *code_line);

// -----------------------------
// Info
// -----------------------------
int rt_get_info(runtime_info_t *out);

#ifdef __cplusplus
}
#endif
