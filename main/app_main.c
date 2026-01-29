#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "tft_console.h"
#include "mpy_runtime.h"

#define BASALT_PROMPT "basalt> "
#define BASALT_INPUT_MAX 128
#define BASALT_PROMPT_COLOR "\x1b[34m"
#define BASALT_COLOR_RESET "\x1b[0m"

static const char *TAG = "basalt";
static char g_cwd[64] = "/";
static int g_uart_num = CONFIG_ESP_CONSOLE_UART_NUM;

static int basalt_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_list ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int n = vprintf(fmt, ap);
    vsnprintf(buf, sizeof(buf), fmt, ap2);
    va_end(ap2);
    va_end(ap);
    if (tft_console_is_ready()) {
        tft_console_write(buf);
    }
    return n;
}

static int basalt_uart_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

static void basalt_tft_print(const char *fmt, ...) {
    if (!tft_console_is_ready()) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    tft_console_write(buf);
}

static void bsh_print_help(bool verbose) {
    if (!verbose) {
        basalt_printf("Basalt shell. Try 'help -commands' for a command list.\n");
        return;
    }
    basalt_printf("Commands: help, ls, cat, cd, pwd, mkdir, cp, mv, rm, apps, edit, run, stop, kill, install, remove, logs, wifi, reboot\n");
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

    DIR *dir = opendir("/data/apps");
    if (!dir) return false;
    bool copied = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, prefix, prefix_len) != 0) continue;
        char src[256];
        int src_len = snprintf(src, sizeof(src), "/data/apps/%s", ent->d_name);
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

    DIR *dir = opendir("/data/apps");
    if (!dir) return false;
    bool removed = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, prefix, prefix_len) != 0) continue;
        char full[256];
        int full_len = snprintf(full, sizeof(full), "/data/apps/%s", ent->d_name);
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
        int dlen = snprintf(dst, sizeof(dst), "/data/apps/%s/%s", app_name, entry);
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
        if (strcmp(start, "..") == 0) {
            if (count > 0) count--;
            continue;
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
        snprintf(out, out_len, "%s", vpath);
        return true;
    }
    if (strncmp(vpath, "/apps", 5) == 0) {
        if (vpath[5] == '\0') {
            snprintf(out, out_len, "/data/apps");
        } else {
            snprintf(out, out_len, "/data/apps%s", vpath + 5);
        }
        return true;
    }
    if (strncmp(vpath, "/sd", 3) == 0) {
        snprintf(out, out_len, "%s", vpath);
        return true;
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
        strcpy(g_cwd, "/");
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
    strncpy(g_cwd, vpath, sizeof(g_cwd) - 1);
    g_cwd[sizeof(g_cwd) - 1] = '\0';
}

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
    if (!path_is_file(src_real) && strncmp(src_real, "/data/apps/", 11) == 0) {
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
    if (!path_is_file(src_real) && strncmp(src_real, "/data/apps/", 11) == 0) {
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

static void bsh_cmd_apps(void) {
    DIR *dir = opendir("/data/apps");
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
        snprintf(toml, sizeof(toml), "/data/apps/%s/app.toml", seen[i]);
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
        snprintf(dst_real, sizeof(dst_real), "/data/apps/%s", app_name);
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
    snprintf(dst_file, sizeof(dst_file), "/data/apps/%s/main.py", app_name);
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
    DIR *dir = opendir("/data/apps");
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
            int full_len = snprintf(full, sizeof(full), "/data/apps/%s", ent->d_name);
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

static void bsh_handle_line(char *line) {
    char *cmd = strtok(line, " \t\r\n");
    if (!cmd) return;

    if (strcmp(cmd, "help") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bool verbose = (arg && (strcmp(arg, "-commands") == 0 || strcmp(arg, "commands") == 0));
        bsh_print_help(verbose);
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
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_mkdir(arg);
    } else if (strcmp(cmd, "cp") == 0) {
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_cp(src, dst, recursive);
    } else if (strcmp(cmd, "mv") == 0) {
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_mv(src, dst, recursive);
    } else if (strcmp(cmd, "rm") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bool recursive = false;
        if (arg && strcmp(arg, "-r") == 0) {
            recursive = true;
            arg = strtok(NULL, " \t\r\n");
        }
        bsh_cmd_rm(arg, recursive);
    } else if (strcmp(cmd, "apps") == 0) {
        bsh_cmd_apps();
    } else if (strcmp(cmd, "edit") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_edit(arg);
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
        char *src = strtok(NULL, " \t\r\n");
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_install(src, name);
    } else if (strcmp(cmd, "remove") == 0) {
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_remove(name);
    } else if (strcmp(cmd, "logs") == 0) {
        basalt_printf("logs: (stub)\n");
    } else if (strcmp(cmd, "wifi") == 0) {
        basalt_printf("wifi: (stub)\n");
    } else if (strcmp(cmd, "reboot") == 0) {
        basalt_printf("rebooting...\n");
        fflush(stdout);
        esp_restart();
    } else {
        basalt_printf("unknown command: %s\n", cmd);
    }
}

static void bsh_task(void *arg) {
    char line[BASALT_INPUT_MAX];

    while (true) {
        basalt_uart_printf(BASALT_PROMPT_COLOR BASALT_PROMPT BASALT_COLOR_RESET);
        tft_console_set_color(0x001F);
        basalt_tft_print(BASALT_PROMPT);
        tft_console_set_color(0xFFFF);
        if (!fgets(line, sizeof(line), stdin)) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        // Echo user input to both UART and TFT.
        basalt_uart_printf("%s", line);
        basalt_tft_print("%s", line);
        bsh_handle_line(line);
    }
}

static void basalt_console_init(void) {
    const int uart_num = CONFIG_ESP_CONSOLE_UART_NUM;
    const int baud = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
    g_uart_num = uart_num;

    uart_driver_install(uart_num, 256, 0, 0, NULL, 0);
    uart_param_config(uart_num, &(uart_config_t){
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    });

    esp_vfs_dev_uart_use_driver(uart_num);
}

static void basalt_fs_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/data",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return;
    }

    DIR *dir = opendir("/data");
    if (dir) {
        closedir(dir);
    } else {
        ESP_LOGW(TAG, "SPIFFS not accessible at /data");
    }

    // Ensure /apps exists for listing apps (virtual /apps maps to /data/apps)
    DIR *apps = opendir("/data/apps");
    if (!apps) {
        mkdir("/data/apps", 0775);
    } else {
        closedir(apps);
    }
}

static void basalt_sd_init(void) {
    // Default SPI SD pins (VSPI): MOSI=23, MISO=19, SCLK=18, CS=5
    const int sd_mosi = 23;
    const int sd_miso = 19;
    const int sd_sclk = 18;
    const int sd_cs = 5;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = sd_mosi,
        .miso_io_num = sd_miso,
        .sclk_io_num = sd_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SD SPI bus init failed: %s", esp_err_to_name(ret));
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = sd_cs;
    slot_config.host_id = VSPI_HOST;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card = NULL;
    ret = esp_vfs_fat_sdspi_mount("/sd", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted at /sd");
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
    tft_console_init();
    mpy_runtime_init();

    basalt_printf("Basalt OS booted. Type 'help'.\n");

    xTaskCreate(bsh_task, "bsh", 4096, NULL, 5, NULL);

    // Keep app_main alive so the main task doesn't exit.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
