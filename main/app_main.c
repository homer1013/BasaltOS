/* BasaltOS generated configuration (do not hardcode pins in app_main.c) */
#include "../config/generated/basalt_config.h"  // generated: feature gates + board pins

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "smoke_test.h"

// Pull in CONFIG_* macros (e.g. CONFIG_CONSOLE_UART_NUM)
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_private/esp_clk.h"
#include "esp_rom_uart.h"
#include "hal/uart_ll.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "tft_console.h"
#include "mpy_runtime.h"

#define BASALT_PROMPT "basalt> "
#define BASALT_INPUT_MAX 128
#define BASALT_PROMPT_COLOR "\x1b[34m"
#define BASALT_COLOR_RESET "\x1b[0m"
#define BASALT_TFT_LOGS 1

#if BASALT_ENABLE_FS_SPIFFS
#define BASALT_DATA_ROOT BASALT_FS_SPIFFS_MOUNT_POINT
#else
#define BASALT_DATA_ROOT "/data"
#endif
#define BASALT_APPS_ROOT BASALT_DATA_ROOT "/apps"
#define BASALT_APPS_PREFIX BASALT_DATA_ROOT "/apps/"
#define BASALT_DEV_APPS_ROOT BASALT_DATA_ROOT "/apps_dev"
#define BASALT_DEV_APPS_PREFIX BASALT_DATA_ROOT "/apps_dev/"
#if BASALT_ENABLE_FS_SPIFFS
#define BASALT_HOME_VPATH BASALT_DATA_ROOT
#else
#define BASALT_HOME_VPATH "/"
#endif

#if defined(BASALT_ENABLE_SHELL_FULL)
#define BASALT_SHELL_LEVEL 2
#elif defined(BASALT_ENABLE_SHELL_MIN)
#define BASALT_SHELL_LEVEL 1
#else
#define BASALT_SHELL_LEVEL 0
#endif
#define BASALT_ENABLE_SHELL (BASALT_SHELL_LEVEL > 0)
#define BASALT_SHELL_BACKEND (BASALT_ENABLE_UART || BASALT_ENABLE_TFT)

static const char *TAG = "basalt";
static int g_uart_num = CONFIG_ESP_CONSOLE_UART_NUM;

static void basalt_uart_write(const char *buf, int len) {
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) {
        esp_rom_uart_putc((char)buf[i]);
    }
}

static int basalt_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return n;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    if (g_uart_num >= 0) basalt_uart_write(buf, n);
#if BASALT_TFT_LOGS
    if (tft_console_is_ready()) {
        tft_console_write(buf);
    }
#endif
    return n;
}

static int basalt_uart_printf(const char *fmt, ...) {
    // Avoid stdio/vfs path (can crash if VFS context is corrupted).
    // Format into a small stack buffer and write directly via the UART driver.
    char buf[256];

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) return n;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;

    basalt_uart_write(buf, n);
    return n;
}

static void basalt_tft_print(const char *fmt, ...) {
    if (!tft_console_is_ready()) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
#if BASALT_TFT_LOGS
    tft_console_write(buf);
#endif
}

#if BASALT_ENABLE_SHELL

typedef struct {
    const char *name;
    const char *usage;
    const char *desc;
} bsh_cmd_help_t;

static const bsh_cmd_help_t k_bsh_help[] = {
    {"help", "help [command|-commands]", "Show help, or details for one command"},
    {"ls", "ls [path]", "List directory contents"},
    {"cat", "cat <file>", "Print a file"},
    {"cd", "cd [path|-|~]", "Change directory (use '-' for previous)"},
    {"pwd", "pwd", "Print current directory"},
#if BASALT_SHELL_LEVEL >= 2
    {"mkdir", "mkdir <path>", "Create directory"},
    {"cp", "cp [-r] <src> <dst>", "Copy file or directory"},
    {"mv", "mv [-r] <src> <dst>", "Move/rename file or directory"},
    {"rm", "rm [-r] <path>", "Remove file or directory"},
#endif
    {"apps", "apps", "List installed apps"},
    {"apps_dev", "apps_dev", "List dev apps in /apps_dev"},
    {"led_test", "led_test [pin]", "Blink/test LED pins (helps identify working LED pin)"},
    {"devcheck", "devcheck [full]", "Quick sanity checks for LED, UI, and filesystems"},
#if BASALT_SHELL_LEVEL >= 2
    {"edit", "edit <file>", "Simple line editor (.save/.quit)"},
#endif
    {"run", "run <app|script>", "Run an app or script"},
    {"run_dev", "run_dev <app|script>", "Run a dev app from /apps_dev"},
    {"stop", "stop", "Stop the running app"},
    {"kill", "kill", "Force stop the running app"},
#if BASALT_SHELL_LEVEL >= 2
    {"install", "install <src> [name]", "Install app from folder or zip"},
    {"remove", "remove <app>", "Remove installed app"},
    {"logs", "logs", "Show logs (stub)"},
    {"wifi", "wifi", "WiFi tools (stub)"},
#endif
    {"reboot", "reboot", "Restart the device"},
};

static const bsh_cmd_help_t *bsh_find_help(const char *name) {
    if (!name) return NULL;
    for (size_t i = 0; i < sizeof(k_bsh_help) / sizeof(k_bsh_help[0]); ++i) {
        if (strcmp(k_bsh_help[i].name, name) == 0) return &k_bsh_help[i];
    }
    return NULL;
}

static char g_cwd[64] = "/";
static char g_prev_cwd[64] = "/";

static void bsh_print_help(bool verbose) {
    if (!verbose) {
        basalt_printf("Basalt shell. Type 'help -commands' for the list, or 'help <cmd>' for details.\n");
        return;
    }
    basalt_printf("Shell level: %s\n\n", BASALT_SHELL_LEVEL >= 2 ? "full" : "minimal");
    for (size_t i = 0; i < sizeof(k_bsh_help) / sizeof(k_bsh_help[0]); ++i) {
        basalt_printf("%-8s  %-24s  %s\n", k_bsh_help[i].name, k_bsh_help[i].usage, k_bsh_help[i].desc);
    }
    basalt_printf("\nVirtual roots: /data (storage), /apps (apps), /apps_dev (dev apps), /sd (SD card)\n");
    basalt_printf("Path tips: ~ (home), - (previous dir), ... (up multiple)\n");
}

static bool path_is_absolute(const char *path) {
    return path && path[0] == '/';
}

static bool is_virtual_root(const char *path) {
    return path && strcmp(path, "/") == 0;
}

static void bsh_list_root(void) {
    basalt_printf("data\n");
    basalt_printf("apps\n");
    basalt_printf("sd\n");
    basalt_printf("apps_dev\n");
}

static bool path_is_dir(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

static bool path_is_file(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
}

static const char *path_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static size_t normalize_name_key(const char *in, char *out, size_t out_len) {
    if (!in || !out || out_len == 0) return 0;
    size_t n = 0;
    for (const char *p = in; *p && n + 1 < out_len; ++p) {
        char c = *p;
        if (c == '~') break;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            out[n++] = c;
        }
    }
    out[n] = '\0';
    return n;
}

static bool resolve_dev_app_in_dir(const char *dir, const char *name, char *out, size_t out_len) {
    if (!dir || !name || !out || out_len == 0) return false;
    DIR *d = opendir(dir);
    if (!d) return false;
    char want[32];
    normalize_name_key(name, want, sizeof(want));
    struct dirent *ent;
    bool found = false;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char cand[32];
        normalize_name_key(ent->d_name, cand, sizeof(cand));
        if (cand[0] == '\0') continue;
        if (strcmp(cand, want) == 0 ||
            (strncmp(cand, want, strlen(cand)) == 0) ||
            (strncmp(want, cand, strlen(want)) == 0)) {
            snprintf(out, out_len, "%s/%s", dir, ent->d_name);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

static void trim_ws(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static bool toml_get_value(const char *toml_path, const char *key, char *out, size_t out_len) {
    FILE *f = fopen(toml_path, "r");
    if (!f) return false;
    char line[128];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        trim_ws(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }
        size_t key_len = strlen(key);
        if (strncmp(line, key, key_len) != 0) {
            continue;
        }
        char *eq = strchr(line, '=');
        if (!eq) continue;
        char *val = eq + 1;
        trim_ws(val);
        if (val[0] == '\"') {
            val++;
            char *endq = strchr(val, '\"');
            if (endq) *endq = '\0';
        }
        if (val[0] == '\0') continue;
        snprintf(out, out_len, "%s", val);
        found = true;
        break;
    }
    fclose(f);
    return found;
}

static bool parse_entry_from_toml(const char *toml_path, char *out, size_t out_len) {
    return toml_get_value(toml_path, "entry", out, out_len);
}

static bool app_entry_path_flat(const char *app_prefix, char *out, size_t out_len) {
    char toml[128];
    snprintf(toml, sizeof(toml), "%s/app.toml", app_prefix);
    char entry[64] = "main.py";
    parse_entry_from_toml(toml, entry, sizeof(entry));
    snprintf(out, out_len, "%s/%s", app_prefix, entry);
    return true;
}

static bool copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return false;
        }
    }
    fclose(in);
    fclose(out);
    return true;
}

static bool copy_dir(const char *src, const char *dst) {
    if (mkdir(dst, 0775) != 0 && errno != EEXIST) {
        return false;
    }
    DIR *dir = opendir(src);
    if (!dir) return false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char s_path[256];
        char d_path[256];
        int s_len = snprintf(s_path, sizeof(s_path), "%s/%s", src, ent->d_name);
        int d_len = snprintf(d_path, sizeof(d_path), "%s/%s", dst, ent->d_name);
        if (s_len < 0 || d_len < 0 || s_len >= (int)sizeof(s_path) || d_len >= (int)sizeof(d_path)) {
            closedir(dir);
            return false;
        }
        if (path_is_dir(s_path)) {
            if (!copy_dir(s_path, d_path)) {
                closedir(dir);
                return false;
            }
        } else {
            if (!copy_file(s_path, d_path)) {
                closedir(dir);
                return false;
            }
        }
    }
    closedir(dir);
    return true;
}

static bool remove_dir_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char p[256];
        int p_len = snprintf(p, sizeof(p), "%s/%s", path, ent->d_name);
        if (p_len < 0 || p_len >= (int)sizeof(p)) {
            closedir(dir);
            return false;
        }
        if (path_is_dir(p)) {
            if (!remove_dir_recursive(p)) {
                closedir(dir);
                return false;
            }
        } else {
            if (unlink(p) != 0) {
                closedir(dir);
                return false;
            }
        }
    }
    closedir(dir);
    return rmdir(path) == 0;
}

static bool ensure_dir(const char *path) {
    if (path_is_dir(path)) return true;
    return mkdir(path, 0775) == 0;
}

static bool copy_app_prefix(const char *src_prefix, const char *dst_dir) {
    // Copy all files in /data/apps with prefix "<app>/"
    const char *app = path_basename(src_prefix);
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "%s/", app);
    size_t prefix_len = strlen(prefix);

    if (!ensure_dir(dst_dir)) return false;

    DIR *dir = opendir(BASALT_APPS_ROOT);
    if (!dir) return false;
    bool copied = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, prefix, prefix_len) != 0) continue;
        char src[256];
        int src_len = snprintf(src, sizeof(src), BASALT_APPS_ROOT "/%s", ent->d_name);
        if (src_len < 0 || src_len >= (int)sizeof(src)) continue;
        const char *rel = ent->d_name + prefix_len;
        char dst[256];
        int dst_len = snprintf(dst, sizeof(dst), "%s/%s", dst_dir, rel);
        if (dst_len < 0 || dst_len >= (int)sizeof(dst)) continue;
        if (copy_file(src, dst)) copied = true;
    }
    closedir(dir);
    return copied;
}

static bool remove_app_prefix(const char *src_prefix) {
    const char *app = path_basename(src_prefix);
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "%s/", app);
    size_t prefix_len = strlen(prefix);

    DIR *dir = opendir(BASALT_APPS_ROOT);
    if (!dir) return false;
    bool removed = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, prefix, prefix_len) != 0) continue;
        char full[256];
        int full_len = snprintf(full, sizeof(full), BASALT_APPS_ROOT "/%s", ent->d_name);
        if (full_len < 0 || full_len >= (int)sizeof(full)) {
            continue;
        }
        if (unlink(full) == 0) removed = true;
    }
    closedir(dir);
    return removed;
}

static bool has_suffix(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t slen = strlen(str);
    size_t tlen = strlen(suffix);
    if (tlen > slen) return false;
    return strcmp(str + (slen - tlen), suffix) == 0;
}

static bool zip_read_u16(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) return false;
    *out = (uint16_t)(b[0] | (b[1] << 8));
    return true;
}

static bool zip_read_u32(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    *out = (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    return true;
}

static bool zip_install_store(const char *zip_path, const char *app_name, char *err, size_t err_len) {
    FILE *f = fopen(zip_path, "rb");
    if (!f) {
        if (err && err_len) snprintf(err, err_len, "cannot open %s", zip_path);
        return false;
    }
    while (true) {
        uint32_t sig = 0;
        if (!zip_read_u32(f, &sig)) break; // EOF
        if (sig != 0x04034b50) break;

        uint16_t ver = 0, flag = 0, method = 0;
        uint16_t mod_time = 0, mod_date = 0;
        uint32_t crc = 0, comp_size = 0, uncomp_size = 0;
        uint16_t fname_len = 0, extra_len = 0;
        (void)ver; (void)mod_time; (void)mod_date; (void)crc; (void)uncomp_size;

        if (!zip_read_u16(f, &ver) ||
            !zip_read_u16(f, &flag) ||
            !zip_read_u16(f, &method) ||
            !zip_read_u16(f, &mod_time) ||
            !zip_read_u16(f, &mod_date) ||
            !zip_read_u32(f, &crc) ||
            !zip_read_u32(f, &comp_size) ||
            !zip_read_u32(f, &uncomp_size) ||
            !zip_read_u16(f, &fname_len) ||
            !zip_read_u16(f, &extra_len)) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip header read failed");
            return false;
        }

        if (flag & 0x08) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip data descriptor not supported");
            return false;
        }
        if (method != 0) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip compression not supported");
            return false;
        }

        char name_buf[128];
        if (fname_len >= sizeof(name_buf)) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip name too long");
            return false;
        }
        if (fread(name_buf, 1, fname_len, f) != fname_len) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip name read failed");
            return false;
        }
        name_buf[fname_len] = '\0';
        for (char *p = name_buf; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        if (strstr(name_buf, "..") || name_buf[0] == '/') {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip path not allowed");
            return false;
        }

        if (extra_len) {
            fseek(f, extra_len, SEEK_CUR);
        }

        bool is_dir = (fname_len > 0 && name_buf[fname_len - 1] == '/');
        if (is_dir) {
            fseek(f, comp_size, SEEK_CUR);
            continue;
        }

        const char *entry = name_buf;
        const char *slash = strchr(name_buf, '/');
        if (slash && slash[1] != '\0') {
            entry = slash + 1;
        }

        char dst[256];
        int dlen = snprintf(dst, sizeof(dst), BASALT_APPS_ROOT "/%s/%s", app_name, entry);
        if (dlen < 0 || dlen >= (int)sizeof(dst)) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "zip path too long");
            return false;
        }

        FILE *out = fopen(dst, "wb");
        if (!out) {
            fclose(f);
            if (err && err_len) snprintf(err, err_len, "cannot write %s", dst);
            return false;
        }
        uint8_t buf[256];
        uint32_t left = comp_size;
        while (left > 0) {
            size_t chunk = left > sizeof(buf) ? sizeof(buf) : left;
            if (fread(buf, 1, chunk, f) != chunk) {
                fclose(out);
                fclose(f);
                if (err && err_len) snprintf(err, err_len, "zip read failed");
                return false;
            }
            if (fwrite(buf, 1, chunk, out) != chunk) {
                fclose(out);
                fclose(f);
                if (err && err_len) snprintf(err, err_len, "zip write failed");
                return false;
            }
            left -= chunk;
        }
        fclose(out);
    }
    fclose(f);
    return true;
}


static void join_path(const char *base, const char *path, char *out, size_t out_len) {
    if (path_is_absolute(path)) {
        snprintf(out, out_len, "%s", path);
        return;
    }
    if (strcmp(base, "/") == 0) {
        snprintf(out, out_len, "/%s", path);
    } else {
        snprintf(out, out_len, "%s/%s", base, path);
    }
}

static void normalize_path(char *path) {
    // In-place normalization: collapse //, handle ./ and ../ segments.
    char *parts[16];
    int count = 0;
    char *p = path;

    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        char *start = p;
        while (*p && *p != '/') p++;
        if (*p) *p++ = '\0';

        if (strcmp(start, ".") == 0) {
            continue;
        }
        if (start[0] == '.' && start[1] == '.') {
            bool all_dots = true;
            for (char *s = start; *s; ++s) {
                if (*s != '.') { all_dots = false; break; }
            }
            if (all_dots) {
                int up = (int)strlen(start) - 1; // ".." -> 1, "..." -> 2
                while (up-- > 0 && count > 0) count--;
                continue;
            }
        }
        if (count < (int)(sizeof(parts) / sizeof(parts[0]))) {
            parts[count++] = start;
        }
    }

    if (count == 0) {
        strcpy(path, "/");
        return;
    }

    char tmp[128] = "/";
    for (int i = 0; i < count; i++) {
        strcat(tmp, parts[i]);
        if (i != count - 1) strcat(tmp, "/");
    }
    strcpy(path, tmp);
}

static void resolve_virtual_path(const char *input, char *out, size_t out_len) {
    char tmp[128];
    const char *src = (input && input[0]) ? input : g_cwd;

    if (src[0] == '~') {
        if (src[1] == '\0') {
            snprintf(tmp, sizeof(tmp), "%s", BASALT_HOME_VPATH);
        } else if (src[1] == '/') {
            snprintf(tmp, sizeof(tmp), "%s%s", BASALT_HOME_VPATH, src + 1);
        } else {
            snprintf(tmp, sizeof(tmp), "%s", src);
        }
        normalize_path(tmp);
        snprintf(out, out_len, "%s", tmp);
        return;
    }

    if (path_is_absolute(src)) {
        snprintf(tmp, sizeof(tmp), "%s", src);
    } else {
        join_path(g_cwd, src, tmp, sizeof(tmp));
    }
    normalize_path(tmp);
    snprintf(out, out_len, "%s", tmp);
}

static bool map_virtual_to_real(const char *vpath, char *out, size_t out_len) {
    if (is_virtual_root(vpath)) {
        snprintf(out, out_len, "/");
        return true;
    }
    if (strncmp(vpath, "/data", 5) == 0) {
#if BASALT_ENABLE_FS_SPIFFS
        if (vpath[5] == '\0') {
            snprintf(out, out_len, "%s", BASALT_FS_SPIFFS_MOUNT_POINT);
        } else {
            snprintf(out, out_len, "%s%s", BASALT_FS_SPIFFS_MOUNT_POINT, vpath + 5);
        }
        return true;
#else
        return false;
#endif
    }
    if (strncmp(vpath, "/apps", 5) == 0) {
        if (vpath[5] == '\0') {
            snprintf(out, out_len, "%s", BASALT_APPS_ROOT);
        } else {
            snprintf(out, out_len, "%s%s", BASALT_APPS_ROOT, vpath + 5);
        }
        return true;
    }
    if (strncmp(vpath, "/apps_dev", 9) == 0) {
        if (vpath[9] == '\0') {
            snprintf(out, out_len, "%s", BASALT_DEV_APPS_ROOT);
        } else {
            snprintf(out, out_len, "%s%s", BASALT_DEV_APPS_ROOT, vpath + 9);
        }
        return true;
    }
    if (strncmp(vpath, "/sd", 3) == 0) {
#if BASALT_ENABLE_FS_SD
        if (vpath[3] == '\0') {
            snprintf(out, out_len, "%s", BASALT_FS_SD_MOUNT_POINT);
        } else {
            snprintf(out, out_len, "%s%s", BASALT_FS_SD_MOUNT_POINT, vpath + 3);
        }
        return true;
#else
        return false;
#endif
    }
    return false;
}

static bool resolve_real_path(const char *input, char *out, size_t out_len) {
    char vpath[128];
    resolve_virtual_path(input, vpath, sizeof(vpath));
    if (!map_virtual_to_real(vpath, out, out_len)) {
        return false;
    }
    return true;
}

#if BASALT_SHELL_LEVEL >= 2
static void bsh_cmd_edit(const char *path) {
    if (!path || !path[0]) {
        basalt_printf("edit: missing file path\n");
        return;
    }
    char rpath[128];
    if (!resolve_real_path(path, rpath, sizeof(rpath))) {
        basalt_printf("edit: invalid path %s\n", path);
        return;
    }

    basalt_printf("edit: type lines, end with .save or .quit\n");
    FILE *f = fopen(rpath, "w");
    if (!f) {
        basalt_printf("edit: cannot open %s\n", path);
        return;
    }

    char line[BASALT_INPUT_MAX];
    while (true) {
        basalt_printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            continue;
        }
        basalt_uart_printf("%s", line);
        basalt_tft_print("%s", line);
        if (strcmp(line, ".save\n") == 0 || strcmp(line, ".save\r\n") == 0) {
            fclose(f);
            basalt_printf("saved %s\n", path);
            return;
        }
        if (strcmp(line, ".quit\n") == 0 || strcmp(line, ".quit\r\n") == 0) {
            fclose(f);
            basalt_printf("aborted %s\n", path);
            return;
        }
        fputs(line, f);
    }
}
#endif

static void bsh_cmd_ls(const char *path) {
    char vpath[128];
    char rpath[128];
    resolve_virtual_path(path, vpath, sizeof(vpath));

    if (is_virtual_root(vpath)) {
        bsh_list_root();
        return;
    }
    if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
        basalt_printf("ls: cannot open %s\n", vpath);
        return;
    }
    DIR *dir = opendir(rpath);
    if (!dir) {
        basalt_printf("ls: cannot open %s\n", vpath);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        basalt_printf("%s\n", ent->d_name);
    }
    closedir(dir);
}

static void bsh_cmd_cat(const char *path) {
    if (!path || !path[0]) {
        basalt_printf("cat: missing file path\n");
        return;
    }
    char vpath[128];
    char rpath[128];
    resolve_virtual_path(path, vpath, sizeof(vpath));
    if (is_virtual_root(vpath)) {
        basalt_printf("cat: %s is a directory\n", vpath);
        return;
    }
    if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
        basalt_printf("cat: cannot open %s\n", vpath);
        return;
    }
    FILE *f = fopen(rpath, "r");
    if (!f) {
        basalt_printf("cat: cannot open %s\n", vpath);
        return;
    }
    char read_buf[128];
    while (fgets(read_buf, sizeof(read_buf), f)) {
        basalt_printf("%s", read_buf);
    }
    fclose(f);
}

static void bsh_cmd_pwd(void) {
    basalt_printf("%s\n", g_cwd);
}

static void bsh_cmd_cd(const char *path) {
    if (!path || !path[0]) {
        strncpy(g_prev_cwd, g_cwd, sizeof(g_prev_cwd) - 1);
        g_prev_cwd[sizeof(g_prev_cwd) - 1] = '\0';
        strncpy(g_cwd, BASALT_HOME_VPATH, sizeof(g_cwd) - 1);
        g_cwd[sizeof(g_cwd) - 1] = '\0';
        return;
    }
    if (strcmp(path, "-") == 0) {
        char tmp[64];
        strncpy(tmp, g_cwd, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        strncpy(g_cwd, g_prev_cwd, sizeof(g_cwd) - 1);
        g_cwd[sizeof(g_cwd) - 1] = '\0';
        strncpy(g_prev_cwd, tmp, sizeof(g_prev_cwd) - 1);
        g_prev_cwd[sizeof(g_prev_cwd) - 1] = '\0';
        return;
    }
    char vpath[128];
    char rpath[128];
    resolve_virtual_path(path, vpath, sizeof(vpath));

    if (is_virtual_root(vpath)) {
        strcpy(g_cwd, "/");
        return;
    }
    if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
        basalt_printf("cd: no such directory: %s\n", vpath);
        return;
    }
    DIR *dir = opendir(rpath);
    if (!dir) {
        basalt_printf("cd: no such directory: %s\n", vpath);
        return;
    }
    closedir(dir);
    strncpy(g_prev_cwd, g_cwd, sizeof(g_prev_cwd) - 1);
    g_prev_cwd[sizeof(g_prev_cwd) - 1] = '\0';
    strncpy(g_cwd, vpath, sizeof(g_cwd) - 1);
    g_cwd[sizeof(g_cwd) - 1] = '\0';
}

#if BASALT_SHELL_LEVEL >= 2
static void bsh_cmd_mkdir(const char *path) {
    if (!path || !path[0]) {
        basalt_printf("mkdir: missing path\n");
        return;
    }
    char rpath[128];
    if (!resolve_real_path(path, rpath, sizeof(rpath))) {
        basalt_printf("mkdir: invalid path %s\n", path);
        return;
    }
    if (mkdir(rpath, 0775) != 0) {
        basalt_printf("mkdir: failed %s\n", path);
        return;
    }
    basalt_printf("created %s\n", path);
}

static void bsh_cmd_cp(const char *src, const char *dst, bool recursive) {
    if (!src || !dst || !src[0] || !dst[0]) {
        basalt_printf("cp: usage cp [-r] <src> <dst>\n");
        return;
    }
    char src_real[128];
    char dst_real[128];
    if (!resolve_real_path(src, src_real, sizeof(src_real))) {
        basalt_printf("cp: invalid src %s\n", src);
        return;
    }
    if (!resolve_real_path(dst, dst_real, sizeof(dst_real))) {
        basalt_printf("cp: invalid dst %s\n", dst);
        return;
    }

    if (path_is_dir(src_real)) {
        if (!recursive) {
            basalt_printf("cp: omitting directory (use -r)\n");
            return;
        }
        if (!copy_dir(src_real, dst_real)) {
            basalt_printf("cp: failed %s\n", src);
            return;
        }
        basalt_printf("copied %s -> %s\n", src, dst);
        return;
    }

    // SPIFFS flat app prefix copy: cp /apps/<name> /sd/<name>
    if (!path_is_file(src_real) && strncmp(src_real, BASALT_APPS_PREFIX, strlen(BASALT_APPS_PREFIX)) == 0) {
        if (!copy_app_prefix(src_real, dst_real)) {
            basalt_printf("cp: failed %s\n", src);
            return;
        }
        basalt_printf("copied %s -> %s\n", src, dst);
        return;
    }

    if (path_is_dir(dst_real)) {
        char dst_file[128];
        int n = snprintf(dst_file, sizeof(dst_file), "%s/%s", dst_real, path_basename(src_real));
        if (n < 0 || n >= (int)sizeof(dst_file)) {
            basalt_printf("cp: destination path too long\n");
            return;
        }
        if (!copy_file(src_real, dst_file)) {
            basalt_printf("cp: failed %s\n", src);
            return;
        }
        basalt_printf("copied %s -> %s\n", src, dst);
        return;
    }

    if (!copy_file(src_real, dst_real)) {
        basalt_printf("cp: failed %s\n", src);
        return;
    }
    basalt_printf("copied %s -> %s\n", src, dst);
}

static void bsh_cmd_mv(const char *src, const char *dst, bool recursive) {
    if (!src || !dst || !src[0] || !dst[0]) {
        basalt_printf("mv: usage mv [-r] <src> <dst>\n");
        return;
    }
    char src_real[128];
    char dst_real[128];
    if (!resolve_real_path(src, src_real, sizeof(src_real))) {
        basalt_printf("mv: invalid src %s\n", src);
        return;
    }
    if (!resolve_real_path(dst, dst_real, sizeof(dst_real))) {
        basalt_printf("mv: invalid dst %s\n", dst);
        return;
    }

    if (path_is_dir(src_real) && !recursive) {
        basalt_printf("mv: omitting directory (use -r)\n");
        return;
    }

    // SPIFFS flat app prefix move: mv /apps/<name> <dst>
    if (!path_is_file(src_real) && strncmp(src_real, BASALT_APPS_PREFIX, strlen(BASALT_APPS_PREFIX)) == 0) {
        char dst_dir[128];
        if (path_is_dir(dst_real)) {
            int n = snprintf(dst_dir, sizeof(dst_dir), "%s/%s", dst_real, path_basename(src_real));
            if (n < 0 || n >= (int)sizeof(dst_dir)) {
                basalt_printf("mv: destination path too long\n");
                return;
            }
        } else {
            snprintf(dst_dir, sizeof(dst_dir), "%s", dst_real);
        }
        if (!copy_app_prefix(src_real, dst_dir)) {
            basalt_printf("mv: failed %s\n", src);
            return;
        }
        if (!remove_app_prefix(src_real)) {
            basalt_printf("mv: warning: failed to remove source %s\n", src);
        }
        basalt_printf("moved %s -> %s\n", src, dst);
        return;
    }

    if (rename(src_real, dst_real) == 0) {
        basalt_printf("moved %s -> %s\n", src, dst);
        return;
    }

    if (path_is_dir(src_real)) {
        if (!copy_dir(src_real, dst_real)) {
            basalt_printf("mv: failed %s\n", src);
            return;
        }
        if (!remove_dir_recursive(src_real)) {
            basalt_printf("mv: warning: failed to remove source %s\n", src);
        }
        basalt_printf("moved %s -> %s\n", src, dst);
        return;
    }

    if (!copy_file(src_real, dst_real)) {
        basalt_printf("mv: failed %s\n", src);
        return;
    }
    if (unlink(src_real) != 0) {
        basalt_printf("mv: warning: failed to remove source %s\n", src);
    }
    basalt_printf("moved %s -> %s\n", src, dst);
}

static void bsh_cmd_rm(const char *path, bool recursive) {
    if (!path || !path[0]) {
        basalt_printf("rm: missing path\n");
        return;
    }
    char rpath[128];
    if (!resolve_real_path(path, rpath, sizeof(rpath))) {
        basalt_printf("rm: invalid path %s\n", path);
        return;
    }
    if (path_is_dir(rpath)) {
        if (!recursive) {
            basalt_printf("rm: is a directory (use rm -r)\n");
            return;
        }
        if (!remove_dir_recursive(rpath)) {
            basalt_printf("rm: failed %s\n", path);
            return;
        }
        basalt_printf("deleted %s\n", path);
        return;
    }
    if (unlink(rpath) != 0) {
        basalt_printf("rm: failed %s\n", path);
        return;
    }
    basalt_printf("deleted %s\n", path);
}
#endif

static void bsh_cmd_apps(void) {
    DIR *dir = opendir(BASALT_APPS_ROOT);
    if (!dir) {
        basalt_printf("apps: /apps not found\n");
        return;
    }
    char seen[16][64];
    int seen_count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        char name[64];
        const char *slash = strchr(ent->d_name, '/');
        if (slash) {
            size_t n = slash - ent->d_name;
            if (n >= sizeof(name)) n = sizeof(name) - 1;
            memcpy(name, ent->d_name, n);
            name[n] = '\0';
        } else {
            size_t n = strnlen(ent->d_name, sizeof(name) - 1);
            memcpy(name, ent->d_name, n);
            name[n] = '\0';
        }
        bool exists = false;
        for (int i = 0; i < seen_count; i++) {
            if (strcmp(seen[i], name) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists && seen_count < (int)(sizeof(seen) / sizeof(seen[0]))) {
            size_t n = strnlen(name, sizeof(seen[0]) - 1);
            memcpy(seen[seen_count], name, n);
            seen[seen_count][n] = '\0';
            seen_count++;
        }
    }
    closedir(dir);
    for (int i = 0; i < seen_count; i++) {
        char toml[128];
        snprintf(toml, sizeof(toml), BASALT_APPS_ROOT "/%s/app.toml", seen[i]);
        char name[64] = {0};
        char version[32] = {0};
        toml_get_value(toml, "name", name, sizeof(name));
        toml_get_value(toml, "version", version, sizeof(version));
        if (name[0] && version[0]) {
            basalt_printf("%s (%s v%s)\n", seen[i], name, version);
        } else if (name[0]) {
            basalt_printf("%s (%s)\n", seen[i], name);
        } else if (version[0]) {
            basalt_printf("%s (v%s)\n", seen[i], version);
        } else {
            basalt_printf("%s\n", seen[i]);
        }
    }
}

static void bsh_cmd_apps_dev(void) {
    DIR *dir = opendir(BASALT_DEV_APPS_ROOT);
    if (!dir) {
        basalt_printf("apps_dev: /apps_dev not found\n");
    } else {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            basalt_printf("%s\n", ent->d_name);
        }
        closedir(dir);
    }
    char sd_root[64];
    if (map_virtual_to_real("/sd", sd_root, sizeof(sd_root))) {
        char sd_apps[96];
        snprintf(sd_apps, sizeof(sd_apps), "%s/apps_dev", sd_root);
        DIR *sd_dir = opendir(sd_apps);
        if (sd_dir) {
            basalt_printf("sd/apps_dev:\n");
            struct dirent *ent;
            while ((ent = readdir(sd_dir)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                basalt_printf("%s\n", ent->d_name);
            }
            closedir(sd_dir);
        }
    }
}

static bool bsh_is_number(const char *s) {
    if (!s || !*s) return false;
    char *end = NULL;
    (void)strtol(s, &end, 10);
    return end && *end == '\0';
}

static bool bsh_add_pin_unique(int *pins, int *count, int max_count, int pin) {
    if (!pins || !count || *count >= max_count || pin < 0) return false;
    for (int i = 0; i < *count; ++i) {
        if (pins[i] == pin) return false;
    }
    pins[*count] = pin;
    (*count)++;
    return true;
}

static void bsh_led_test_one_pin(int pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        basalt_printf("led_test: failed to configure GPIO%d\n", pin);
        return;
    }

    basalt_printf("led_test: GPIO%d phase A (HIGH then LOW)\n", pin);
    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1500));
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(1500));

    basalt_printf("led_test: GPIO%d phase B (LOW then HIGH)\n", pin);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(1500));
    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1500));

    gpio_set_level(pin, 0);
    basalt_printf("led_test: GPIO%d done\n", pin);
}

static void bsh_cmd_led_test(const char *arg) {
    int pins[12];
    int count = 0;

    if (arg && arg[0]) {
        if (!bsh_is_number(arg)) {
            basalt_printf("led_test: invalid pin '%s'\n", arg);
            basalt_printf("usage: led_test [pin]\n");
            return;
        }
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), (int)strtol(arg, NULL, 10));
    } else {
#if defined(BASALT_PIN_LED)
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), BASALT_PIN_LED);
#endif
#if defined(BASALT_PIN_LED_R)
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), BASALT_PIN_LED_R);
#endif
#if defined(BASALT_PIN_LED_G)
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), BASALT_PIN_LED_G);
#endif
#if defined(BASALT_PIN_LED_B)
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), BASALT_PIN_LED_B);
#endif
#if defined(BASALT_BOARD_CYD)
        // Known common LED pins on CYD variants.
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), 4);
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), 16);
        bsh_add_pin_unique(pins, &count, (int)(sizeof(pins) / sizeof(pins[0])), 17);
#endif
    }

    if (count == 0) {
        basalt_printf("led_test: no candidate pins available\n");
        return;
    }

    basalt_printf("led_test: testing %d pin(s)\n", count);
    for (int i = 0; i < count; ++i) {
        bsh_led_test_one_pin(pins[i]);
    }
    basalt_printf("led_test: complete\n");
}

static void bsh_cmd_devcheck(const char *arg) {
    const bool full = (arg && strcmp(arg, "full") == 0);
    basalt_printf("devcheck: begin%s\n", full ? " (full)" : "");

    // UI sanity
    basalt_printf("devcheck: tft_console=%s\n", tft_console_is_ready() ? "ready" : "not-ready");

    // Filesystem sanity
    char rpath[128];
    if (map_virtual_to_real(BASALT_DATA_ROOT, rpath, sizeof(rpath)) && path_is_dir(rpath)) {
        basalt_printf("devcheck: %s -> %s [ok]\n", BASALT_DATA_ROOT, rpath);
    } else {
        basalt_printf("devcheck: %s [missing]\n", BASALT_DATA_ROOT);
    }
    if (map_virtual_to_real(BASALT_APPS_ROOT, rpath, sizeof(rpath)) && path_is_dir(rpath)) {
        basalt_printf("devcheck: /apps -> %s [ok]\n", rpath);
    } else {
        basalt_printf("devcheck: /apps [missing]\n");
    }
    if (map_virtual_to_real(BASALT_DEV_APPS_ROOT, rpath, sizeof(rpath)) && path_is_dir(rpath)) {
        basalt_printf("devcheck: /apps_dev -> %s [ok]\n", rpath);
    } else {
        basalt_printf("devcheck: /apps_dev [missing]\n");
    }
    if (map_virtual_to_real("/sd", rpath, sizeof(rpath)) && path_is_dir(rpath)) {
        basalt_printf("devcheck: /sd -> %s [ok]\n", rpath);
    } else {
        basalt_printf("devcheck: /sd [missing or unmounted]\n");
    }

    // LED sanity
    if (full) {
        basalt_printf("devcheck: running led_test full sweep\n");
        bsh_cmd_led_test(NULL);
    } else {
        int pin = -1;
#if defined(BASALT_PIN_LED)
        pin = BASALT_PIN_LED;
#elif defined(BASALT_PIN_LED_G)
        pin = BASALT_PIN_LED_G;
#elif defined(BASALT_PIN_LED_B)
        pin = BASALT_PIN_LED_B;
#elif defined(BASALT_PIN_LED_R)
        pin = BASALT_PIN_LED_R;
#endif
        if (pin >= 0) {
            basalt_printf("devcheck: quick LED pulse on GPIO%d\n", pin);
            bsh_led_test_one_pin(pin);
        } else {
            basalt_printf("devcheck: no default LED pin configured\n");
        }
    }

    basalt_printf("devcheck: done\n");
}

#if BASALT_SHELL_LEVEL >= 2
static void bsh_cmd_install(const char *src, const char *name) {
    if (!src || !src[0]) {
        basalt_printf("install: missing source path\n");
        return;
    }
    char src_real[128];
    if (!resolve_real_path(src, src_real, sizeof(src_real))) {
        basalt_printf("install: invalid path %s\n", src);
        return;
    }
    const char *app_name = name && name[0] ? name : path_basename(src_real);
    if (!app_name || !app_name[0]) {
        basalt_printf("install: invalid app name\n");
        return;
    }
    if (has_suffix(src_real, ".zip")) {
        if (!name || !name[0]) {
            basalt_printf("install: zip requires app name\n");
            return;
        }
        char err[128];
        if (!zip_install_store(src_real, app_name, err, sizeof(err))) {
            basalt_printf("install: %s\n", err);
            return;
        }
        basalt_printf("installed %s\n", app_name);
        return;
    }
    if (path_is_dir(src_real)) {
        char dst_real[128];
        snprintf(dst_real, sizeof(dst_real), BASALT_APPS_ROOT "/%s", app_name);
        if (!copy_dir(src_real, dst_real)) {
            basalt_printf("install: failed to copy %s\n", app_name);
            return;
        }
        basalt_printf("installed %s\n", app_name);
        return;
    }
    if (!path_is_file(src_real)) {
        basalt_printf("install: source not found: %s\n", src);
        return;
    }
    char dst_file[128];
    snprintf(dst_file, sizeof(dst_file), BASALT_APPS_ROOT "/%s/main.py", app_name);
    if (!copy_file(src_real, dst_file)) {
        basalt_printf("install: failed to copy %s\n", app_name);
        return;
    }
    basalt_printf("installed %s\n", app_name);
}

static void bsh_cmd_remove(const char *target) {
    if (!target || !target[0]) {
        basalt_printf("remove: missing app name\n");
        return;
    }
    char vpath[128];
    if (path_is_absolute(target)) {
        resolve_virtual_path(target, vpath, sizeof(vpath));
    } else {
        snprintf(vpath, sizeof(vpath), "/apps/%s", target);
    }
    char rpath[128];
    if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
        basalt_printf("remove: invalid path %s\n", vpath);
        return;
    }
    if (path_is_dir(rpath)) {
        if (!remove_dir_recursive(rpath)) {
            basalt_printf("remove: failed %s\n", vpath);
            return;
        }
        basalt_printf("removed %s\n", vpath);
        return;
    }
    // SPIFFS flat path: remove any file with prefix "<app>/"
    DIR *dir = opendir(BASALT_APPS_ROOT);
    if (!dir) {
        basalt_printf("remove: /apps not found\n");
        return;
    }
    const char *app = path_basename(rpath);
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "%s/", app);
    size_t prefix_len = strlen(prefix);
    bool removed = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, prefix, prefix_len) == 0) {
            char full[256];
            int full_len = snprintf(full, sizeof(full), BASALT_APPS_ROOT "/%s", ent->d_name);
            if (full_len < 0 || full_len >= (int)sizeof(full)) {
                continue;
            }
            if (unlink(full) == 0) removed = true;
        }
    }
    closedir(dir);
    if (!removed) {
        basalt_printf("remove: not found %s\n", vpath);
        return;
    }
    basalt_printf("removed %s\n", vpath);
}
#endif

static void bsh_handle_line(char *line) {
    char *cmd = strtok(line, " \t\r\n");
    if (!cmd) return;

    if (strcmp(cmd, "help") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        if (!arg) {
            bsh_print_help(false);
            return;
        }
        if (strcmp(arg, "-commands") == 0 || strcmp(arg, "commands") == 0) {
            bsh_print_help(true);
            return;
        }
        const bsh_cmd_help_t *h = bsh_find_help(arg);
        if (h) {
            basalt_printf("%s\n  %s\n  %s\n", h->name, h->usage, h->desc);
        } else {
            basalt_printf("help: unknown command: %s\n", arg);
        }
    } else if (strcmp(cmd, "ls") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_ls(arg);
    } else if (strcmp(cmd, "cat") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_cat(arg);
    } else if (strcmp(cmd, "pwd") == 0) {
        bsh_cmd_pwd();
    } else if (strcmp(cmd, "cd") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_cd(arg);
    } else if (strcmp(cmd, "mkdir") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_mkdir(arg);
#else
        basalt_printf("mkdir: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "cp") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_cp(src, dst, recursive);
#else
        basalt_printf("cp: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "mv") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_mv(src, dst, recursive);
#else
        basalt_printf("mv: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "rm") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        char *arg = strtok(NULL, " \t\r\n");
        bool recursive = false;
        if (arg && strcmp(arg, "-r") == 0) {
            recursive = true;
            arg = strtok(NULL, " \t\r\n");
        }
        bsh_cmd_rm(arg, recursive);
#else
        basalt_printf("rm: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "apps") == 0) {
        bsh_cmd_apps();
    } else if (strcmp(cmd, "apps_dev") == 0) {
        bsh_cmd_apps_dev();
    } else if (strcmp(cmd, "led_test") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_led_test(arg);
    } else if (strcmp(cmd, "devcheck") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_devcheck(arg);
    } else if (strcmp(cmd, "edit") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_edit(arg);
#else
        basalt_printf("edit: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "run") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        if (!arg) {
            basalt_printf("run: missing script path\n");
        } else {
            char vpath[128];
            if (!path_is_absolute(arg) && !strchr(arg, '/') && !strstr(arg, ".py")) {
                snprintf(vpath, sizeof(vpath), "/apps/%s", arg);
            } else {
                resolve_virtual_path(arg, vpath, sizeof(vpath));
            }
            char rpath[128];
            if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
                basalt_printf("run: invalid path %s\n", vpath);
                return;
            }
            char script[128];
            if (path_is_dir(rpath)) {
                app_entry_path_flat(rpath, script, sizeof(script));
            } else {
                // SPIFFS flat layout: treat as app prefix
                app_entry_path_flat(rpath, script, sizeof(script));
                if (!path_is_file(script)) {
                    snprintf(script, sizeof(script), "%s", rpath);
                }
            }
            if (!path_is_file(script)) {
                basalt_printf("run: no entry script in %s\n", vpath);
                return;
            }
            char err[128];
            if (!mpy_runtime_start_file(script, err, sizeof(err))) {
                basalt_printf("run: %s\n", err);
            }
        }
    } else if (strcmp(cmd, "run_dev") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        if (!arg) {
            basalt_printf("run_dev: missing script path\n");
        } else {
            const bool is_name = !path_is_absolute(arg) && !strchr(arg, '/') && !strstr(arg, ".py");
            char vpath[128];
            if (path_is_absolute(arg)) {
                resolve_virtual_path(arg, vpath, sizeof(vpath));
            } else if (is_name) {
                snprintf(vpath, sizeof(vpath), "/apps_dev/%s", arg);
            } else {
                resolve_virtual_path(arg, vpath, sizeof(vpath));
            }
            if (strncmp(vpath, "/apps_dev", 9) != 0 && strncmp(vpath, "/sd/apps_dev", 12) != 0) {
                basalt_printf("run_dev: path must be under /apps_dev or /sd/apps_dev\n");
                return;
            }
            char rpath[128];
            if (!map_virtual_to_real(vpath, rpath, sizeof(rpath))) {
                basalt_printf("run_dev: invalid path %s\n", vpath);
                return;
            }
            char script[128];
            if (path_is_dir(rpath)) {
                app_entry_path_flat(rpath, script, sizeof(script));
            } else {
                app_entry_path_flat(rpath, script, sizeof(script));
                if (!path_is_file(script)) {
                    snprintf(script, sizeof(script), "%s", rpath);
                }
            }
            bool tried_sd = false;
            if (!path_is_file(script) && is_name) {
                char vpath_sd[128];
                snprintf(vpath_sd, sizeof(vpath_sd), "/sd/apps_dev/%s", arg);
                char rpath_sd[128];
                if (map_virtual_to_real(vpath_sd, rpath_sd, sizeof(rpath_sd))) {
                    tried_sd = true;
                    if (path_is_dir(rpath_sd)) {
                        app_entry_path_flat(rpath_sd, script, sizeof(script));
                    } else {
                        app_entry_path_flat(rpath_sd, script, sizeof(script));
                        if (!path_is_file(script)) {
                            snprintf(script, sizeof(script), "%s", rpath_sd);
                        }
                    }
                }
            }
            if (!path_is_file(script) && is_name) {
                char rpath_sd_root[128];
                if (map_virtual_to_real("/sd/apps_dev", rpath_sd_root, sizeof(rpath_sd_root))) {
                    char match[128];
                    if (resolve_dev_app_in_dir(rpath_sd_root, arg, match, sizeof(match))) {
                        if (path_is_dir(match)) {
                            app_entry_path_flat(match, script, sizeof(script));
                        } else {
                            app_entry_path_flat(match, script, sizeof(script));
                            if (!path_is_file(script)) {
                                snprintf(script, sizeof(script), "%s", match);
                            }
                        }
                        tried_sd = true;
                    }
                }
            }
            if (!path_is_file(script)) {
                if (tried_sd) {
                    basalt_printf("run_dev: no entry script in %s or /sd/apps_dev/%s\n", vpath, arg);
                } else {
                    basalt_printf("run_dev: no entry script in %s\n", vpath);
                }
                return;
            }
            char err[128];
            if (!mpy_runtime_start_file(script, err, sizeof(err))) {
                basalt_printf("run_dev: %s\n", err);
            }
        }
    } else if (strcmp(cmd, "stop") == 0) {
        char err[128];
        if (!mpy_runtime_stop(false, err, sizeof(err))) {
            basalt_printf("stop: %s\n", err);
        } else {
            basalt_printf("stopped\n");
        }
    } else if (strcmp(cmd, "kill") == 0) {
        char err[128];
        if (!mpy_runtime_stop(true, err, sizeof(err))) {
            basalt_printf("kill: %s\n", err);
        } else {
            basalt_printf("killed\n");
        }
    } else if (strcmp(cmd, "install") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        char *src = strtok(NULL, " \t\r\n");
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_install(src, name);
#else
        basalt_printf("install: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "remove") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_remove(name);
#else
        basalt_printf("remove: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "logs") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        basalt_printf("logs: (stub)\n");
#else
        basalt_printf("logs: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "wifi") == 0) {
#if BASALT_SHELL_LEVEL >= 2
        basalt_printf("wifi: (stub)\n");
#else
        basalt_printf("wifi: disabled in minimal shell\n");
#endif
    } else if (strcmp(cmd, "reboot") == 0) {
        basalt_printf("rebooting...\n");
        fflush(stdout);
        esp_restart();
    } else {
        basalt_printf("unknown command: %s\n", cmd);
    }
}

static int basalt_uart_readline(char *out, size_t out_len) {
    if (!out || out_len == 0) return 0;
    size_t n = 0;
    while (n + 1 < out_len) {
        uint8_t ch = 0;
        if (esp_rom_uart_rx_one_char(&ch) != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            out[n++] = '\n';
            out[n] = '\0';
            return (int)n;
        }
        if (ch == 0x08 || ch == 0x7f) { // backspace/delete
            if (n > 0) n--;
            continue;
        }
        out[n++] = (char)ch;
    }
    out[n] = '\0';
    return (int)n;
}

static void bsh_task(void *arg) {
    char line[BASALT_INPUT_MAX];

    while (true) {
        basalt_uart_printf(BASALT_PROMPT_COLOR BASALT_PROMPT BASALT_COLOR_RESET);
#if BASALT_TFT_LOGS
        tft_console_set_color(0x001F);
        basalt_tft_print(BASALT_PROMPT);
        tft_console_set_color(0xFFFF);
#endif
        int got = basalt_uart_readline(line, sizeof(line));
        if (got <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        // Echo user input to UART (TFT echo can be enabled via BASALT_TFT_LOGS).
        basalt_uart_printf("%s", line);
#if BASALT_TFT_LOGS
        basalt_tft_print("%s", line);
#endif
        bsh_handle_line(line);
    }
}

#endif // BASALT_ENABLE_SHELL

static void basalt_console_init(void) {
#if BASALT_ENABLE_UART
    // Console over UART0 (ESP-IDF console config)
    const int uart_num = CONFIG_ESP_CONSOLE_UART_NUM;
    const int baud = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
    g_uart_num = uart_num;

    // Use ROM UART routines and disable UART interrupts to avoid driver ISR crashes.
    esp_rom_uart_set_as_console(uart_num);
    esp_rom_uart_set_clock_baudrate(uart_num, esp_clk_apb_freq(), baud);
    uart_ll_disable_intr_mask(UART_LL_GET_HW(uart_num), UINT32_MAX);
    uart_ll_clr_intsts_mask(UART_LL_GET_HW(uart_num), UINT32_MAX);

    ESP_LOGI(TAG, "Console: UART%d @ %d", uart_num, baud);
#else
    // UART module disabled by configurator: keep app running, but warn.
    g_uart_num = -1;
    ESP_LOGW(TAG, "Console UART disabled (BASALT_ENABLE_UART=0).");
#endif
}


static void basalt_fs_init(void) {
#if BASALT_ENABLE_FS_SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BASALT_FS_SPIFFS_MOUNT_POINT,
        .partition_label = "storage",
        .max_files = 8,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: total=%u, used=%u", (unsigned)total, (unsigned)used);
    } else {
        ESP_LOGW(TAG, "SPIFFS info failed (%s)", esp_err_to_name(ret));
    }
#else
    ESP_LOGI(TAG, "SPIFFS disabled (BASALT_ENABLE_FS_SPIFFS=0)");
#endif
}


static void basalt_sd_init(void) {
#if BASALT_ENABLE_FS_SD
    // BasaltOS expects the board profile to provide SD card SPI pins.
    // If you enable fs_sd but your board.json doesn't define these, fail fast at compile time.
    #ifndef BASALT_PIN_SD_MOSI
    #error "fs_sd enabled but BASALT_PIN_SD_MOSI is not defined (board.json missing sd_mosi pin)."
    #endif
    #ifndef BASALT_PIN_SD_MISO
    #error "fs_sd enabled but BASALT_PIN_SD_MISO is not defined (board.json missing sd_miso pin)."
    #endif
    #ifndef BASALT_PIN_SD_SCLK
    #error "fs_sd enabled but BASALT_PIN_SD_SCLK is not defined (board.json missing sd_sclk pin)."
    #endif
    #ifndef BASALT_PIN_SD_CS
    #error "fs_sd enabled but BASALT_PIN_SD_CS is not defined (board.json missing sd_cs pin)."
    #endif

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = BASALT_PIN_SD_MOSI,
        .miso_io_num = BASALT_PIN_SD_MISO,
        .sclk_io_num = BASALT_PIN_SD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD SPI bus init failed (%s)", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = BASALT_PIN_SD_CS;
    slot_cfg.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card = NULL;
    ret = esp_vfs_fat_sdspi_mount(BASALT_FS_SD_MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed (%s)", esp_err_to_name(ret));
        spi_bus_free(host.slot);
        return;
    }

    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD mounted at %s", BASALT_FS_SD_MOUNT_POINT);
#else
    ESP_LOGI(TAG, "SD disabled (BASALT_ENABLE_FS_SD=0)");
#endif
}


void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    basalt_console_init();
    basalt_fs_init();
    basalt_sd_init();
#if BASALT_ENABLE_TFT
    tft_console_init();
#else
    ESP_LOGI(TAG, "TFT disabled (BASALT_ENABLE_TFT=0)");
#endif

    (void)basalt_smoke_test_run();

#if BASALT_ENABLE_SHELL
    if (BASALT_SHELL_BACKEND) {
        basalt_printf("Basalt OS booted. Type 'help'.\n");
        xTaskCreate(bsh_task, "bsh", 8192, NULL, 5, NULL);
    } else {
        basalt_printf("Basalt OS booted.\n");
        ESP_LOGI(TAG, "Shell disabled (no backend enabled)");
    }
#else
    basalt_printf("Basalt OS booted.\n");
    ESP_LOGI(TAG, "Shell disabled (BASALT_ENABLE_SHELL=0)");
#endif

    // Keep app_main alive so the main task doesn't exit.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
