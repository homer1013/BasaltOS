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
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_vfs_dev.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_private/esp_clk.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "driver/twai.h"
#include "driver/ledc.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"
#if BASALT_ENABLE_BLUETOOTH && defined(CONFIG_BT_ENABLED) && CONFIG_BT_ENABLED
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#endif

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
#define BASALT_SHELL_LEVEL 3
#elif defined(BASALT_ENABLE_SHELL_MIN)
#define BASALT_SHELL_LEVEL 2
#elif defined(BASALT_ENABLE_SHELL_TINY)
#define BASALT_SHELL_LEVEL 1
#else
#define BASALT_SHELL_LEVEL 0
#endif
#define BASALT_ENABLE_SHELL (BASALT_SHELL_LEVEL > 0)
#define BASALT_SHELL_BACKEND (BASALT_ENABLE_UART || BASALT_ENABLE_TFT)

#ifndef BASALT_APPLET_COUNT
#define BASALT_APPLET_COUNT 0
#endif
#ifndef BASALT_APPLETS_CSV
#define BASALT_APPLETS_CSV ""
#endif
#ifndef BASALT_ENABLE_DISPLAY_SSD1306
#define BASALT_ENABLE_DISPLAY_SSD1306 0
#endif
#ifndef BASALT_ENABLE_RTC
#define BASALT_ENABLE_RTC 0
#endif
#ifndef BASALT_ENABLE_IMU
#define BASALT_ENABLE_IMU 0
#endif
#ifndef BASALT_ENABLE_DHT22
#define BASALT_ENABLE_DHT22 0
#endif
#ifndef BASALT_ENABLE_BME280
#define BASALT_ENABLE_BME280 0
#endif
#ifndef BASALT_ENABLE_ADS1115
#define BASALT_ENABLE_ADS1115 0
#endif
#ifndef BASALT_ENABLE_MIC
#define BASALT_ENABLE_MIC 0
#endif
#ifndef BASALT_ENABLE_TP4056
#define BASALT_ENABLE_TP4056 0
#endif
#ifndef BASALT_ENABLE_I2C
#define BASALT_ENABLE_I2C 0
#endif
#ifndef BASALT_ENABLE_WIFI
#define BASALT_ENABLE_WIFI 0
#endif
#ifndef BASALT_ENABLE_BLUETOOTH
#define BASALT_ENABLE_BLUETOOTH 0
#endif
#ifndef BASALT_ENABLE_TWAI
#define BASALT_ENABLE_TWAI 0
#endif
#ifndef BASALT_ENABLE_MCP2544FD
#define BASALT_ENABLE_MCP2544FD 0
#endif
#ifndef BASALT_ENABLE_MCP2515
#define BASALT_ENABLE_MCP2515 0
#endif
#ifndef BASALT_ENABLE_MCP23017
#define BASALT_ENABLE_MCP23017 0
#endif
#ifndef BASALT_ENABLE_ULN2003
#define BASALT_ENABLE_ULN2003 0
#endif
#ifndef BASALT_ENABLE_L298N
#define BASALT_ENABLE_L298N 0
#endif
#ifndef BASALT_CFG_TWAI_BITRATE
#define BASALT_CFG_TWAI_BITRATE 500000
#endif
#ifndef BASALT_PIN_CAN_TX
#define BASALT_PIN_CAN_TX -1
#endif
#ifndef BASALT_PIN_CAN_RX
#define BASALT_PIN_CAN_RX -1
#endif
#ifndef BASALT_PIN_CAN_STBY
#define BASALT_PIN_CAN_STBY -1
#endif
#ifndef BASALT_PIN_CAN_EN
#define BASALT_PIN_CAN_EN -1
#endif
#ifndef BASALT_PIN_CAN_CS
#define BASALT_PIN_CAN_CS -1
#endif
#ifndef BASALT_PIN_CAN_INT
#define BASALT_PIN_CAN_INT -1
#endif
#ifndef BASALT_PIN_SPI_SCLK
#define BASALT_PIN_SPI_SCLK -1
#endif
#ifndef BASALT_PIN_SPI_MISO
#define BASALT_PIN_SPI_MISO -1
#endif
#ifndef BASALT_PIN_SPI_MOSI
#define BASALT_PIN_SPI_MOSI -1
#endif
#ifndef BASALT_CFG_MCP2544FD_STBY_ACTIVE_HIGH
#define BASALT_CFG_MCP2544FD_STBY_ACTIVE_HIGH 1
#endif
#ifndef BASALT_CFG_MCP2544FD_EN_ACTIVE_HIGH
#define BASALT_CFG_MCP2544FD_EN_ACTIVE_HIGH 1
#endif
#ifndef BASALT_PIN_ULN2003_IN1
#define BASALT_PIN_ULN2003_IN1 -1
#endif
#ifndef BASALT_PIN_ULN2003_IN2
#define BASALT_PIN_ULN2003_IN2 -1
#endif
#ifndef BASALT_PIN_ULN2003_IN3
#define BASALT_PIN_ULN2003_IN3 -1
#endif
#ifndef BASALT_PIN_ULN2003_IN4
#define BASALT_PIN_ULN2003_IN4 -1
#endif
#ifndef BASALT_CFG_ULN2003_ACTIVE_HIGH
#define BASALT_CFG_ULN2003_ACTIVE_HIGH 1
#endif
#ifndef BASALT_CFG_ULN2003_STEP_DELAY_MS
#define BASALT_CFG_ULN2003_STEP_DELAY_MS 10
#endif
#ifndef BASALT_PIN_L298N_IN1
#define BASALT_PIN_L298N_IN1 -1
#endif
#ifndef BASALT_PIN_L298N_IN2
#define BASALT_PIN_L298N_IN2 -1
#endif
#ifndef BASALT_PIN_L298N_ENA
#define BASALT_PIN_L298N_ENA -1
#endif
#ifndef BASALT_PIN_L298N_IN3
#define BASALT_PIN_L298N_IN3 -1
#endif
#ifndef BASALT_PIN_L298N_IN4
#define BASALT_PIN_L298N_IN4 -1
#endif
#ifndef BASALT_PIN_L298N_ENB
#define BASALT_PIN_L298N_ENB -1
#endif
#ifndef BASALT_CFG_L298N_IN_ACTIVE_HIGH
#define BASALT_CFG_L298N_IN_ACTIVE_HIGH 1
#endif
#ifndef BASALT_CFG_L298N_EN_ACTIVE_HIGH
#define BASALT_CFG_L298N_EN_ACTIVE_HIGH 1
#endif
#ifndef BASALT_CFG_L298N_PWM_FREQ_HZ
#define BASALT_CFG_L298N_PWM_FREQ_HZ 20000
#endif
#ifndef BASALT_CFG_L298N_DEFAULT_SPEED_PCT
#define BASALT_CFG_L298N_DEFAULT_SPEED_PCT 100
#endif
#ifndef BASALT_PIN_I2C_SDA
#define BASALT_PIN_I2C_SDA -1
#endif
#ifndef BASALT_PIN_I2C_SCL
#define BASALT_PIN_I2C_SCL -1
#endif
#ifndef BASALT_PIN_DHT22_DATA
#define BASALT_PIN_DHT22_DATA -1
#endif
#ifndef BASALT_PIN_TP4056_CHRG
#define BASALT_PIN_TP4056_CHRG -1
#endif
#ifndef BASALT_PIN_TP4056_DONE
#define BASALT_PIN_TP4056_DONE -1
#endif
#ifndef BASALT_PIN_TP4056_CE
#define BASALT_PIN_TP4056_CE -1
#endif
#ifndef BASALT_CFG_TP4056_STATUS_PINS_ACTIVE_LOW
#define BASALT_CFG_TP4056_STATUS_PINS_ACTIVE_LOW 1
#endif
#ifndef BASALT_CFG_TP4056_CE_PIN_PRESENT
#define BASALT_CFG_TP4056_CE_PIN_PRESENT 0
#endif
#ifndef BASALT_CFG_TP4056_CE_ACTIVE_HIGH
#define BASALT_CFG_TP4056_CE_ACTIVE_HIGH 1
#endif
#ifndef BASALT_CFG_IMU_DRIVER
#ifdef BASALT_IMU_DRIVER
#define BASALT_CFG_IMU_DRIVER BASALT_IMU_DRIVER
#else
#define BASALT_CFG_IMU_DRIVER "MPU6886"
#endif
#ifndef BASALT_CFG_IMU_ADDRESS
#ifdef BASALT_IMU_ADDRESS
#define BASALT_CFG_IMU_ADDRESS BASALT_IMU_ADDRESS
#else
#define BASALT_CFG_IMU_ADDRESS "0x68"
#endif
#endif
#ifndef BASALT_CFG_BME280_I2C_ADDRESS
#define BASALT_CFG_BME280_I2C_ADDRESS "0x76"
#endif
#ifndef BASALT_CFG_DHT22_SAMPLE_MS
#define BASALT_CFG_DHT22_SAMPLE_MS 2000
#endif
#ifndef BASALT_CFG_DHT_SENSOR_TYPE
#define BASALT_CFG_DHT_SENSOR_TYPE "auto"
#endif
#ifndef BASALT_CFG_MCP23017_ADDRESS
#define BASALT_CFG_MCP23017_ADDRESS "0x20"
#endif
#ifndef BASALT_CFG_MCP23017_DEFAULT_DIRECTION
#define BASALT_CFG_MCP23017_DEFAULT_DIRECTION "input"
#endif
#ifndef BASALT_CFG_ADS1115_ADDRESS
#define BASALT_CFG_ADS1115_ADDRESS "0x48"
#endif
#ifndef BASALT_CFG_ADS1115_PGA_MV
#define BASALT_CFG_ADS1115_PGA_MV 2048
#endif
#ifndef BASALT_CFG_ADS1115_SAMPLE_RATE_SPS
#define BASALT_CFG_ADS1115_SAMPLE_RATE_SPS 128
#endif
#ifndef BASALT_CFG_ADS1115_MODE
#define BASALT_CFG_ADS1115_MODE "single_shot"
#endif

static const char *TAG = "basalt";
static int g_uart_num = CONFIG_ESP_CONSOLE_UART_NUM;

static void basalt_uart_write(const char *buf, int len) {
    if (len <= 0 || !buf) return;
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED) && CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
    if (usb_serial_jtag_is_driver_installed()) {
        int off = 0;
        while (off < len) {
            int wrote = usb_serial_jtag_write_bytes(buf + off, (size_t)(len - off), pdMS_TO_TICKS(100));
            if (wrote > 0) {
                off += wrote;
                continue;
            }
            // Give the USB task/host a moment, then retry.
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        (void)usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(50));
        return;
    }
#endif
    (void)g_uart_num;
    (void)fwrite(buf, 1, (size_t)len, stdout);
    fflush(stdout);
}


static int basalt_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return n;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    // Always write through the active console backend (USB-Serial/JTAG or stdio fallback).
    // The shell prompt/echo already does this; command handlers should behave the same.
    basalt_uart_write(buf, n);
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
#if BASALT_SHELL_LEVEL >= 3
    {"mkdir", "mkdir <path>", "Create directory"},
    {"cp", "cp [-r] <src> <dst>", "Copy file or directory"},
    {"mv", "mv [-r] <src> <dst>", "Move/rename file or directory"},
    {"rm", "rm [-r] <path>", "Remove file or directory"},
#endif
    {"apps", "apps", "List installed apps"},
    {"apps_dev", "apps_dev", "List dev apps in /apps_dev"},
    {"led_test", "led_test [pin]", "Blink/test LED pins (helps identify working LED pin)"},
    {"devcheck", "devcheck [full]", "Quick sanity checks for LED, UI, and filesystems"},
    {"drivers", "drivers", "Show configured driver status and implementation level"},
#if BASALT_SHELL_LEVEL >= 3
    {"edit", "edit <file>", "Simple line editor (.save/.quit)"},
#endif
    {"run", "run <app|script>", "Run an app or script"},
    {"run_dev", "run_dev <app|script>", "Run a dev app from /apps_dev"},
    {"stop", "stop", "Stop the running app"},
    {"kill", "kill", "Force stop the running app"},
    {"applet", "applet list|run <name>", "List or run selected applets"},
#if BASALT_SHELL_LEVEL >= 3
    {"install", "install <src> [name]", "Install app from folder or zip"},
    {"remove", "remove <app>", "Remove installed app"},
    {"logs", "logs", "Show runtime diagnostics and last app error"},
    {"imu", "imu [status|read|whoami|stream [hz] [samples]]", "IMU probe/read over configured I2C pins"},
    {"dht22", "dht22 [status|read [pin] [auto|dht22|dht11]]", "DHT22/DHT11-compatible single-wire probe and read"},
    {"bme280", "bme280 [status|probe|read]", "BME280 probe/status/raw-read over configured I2C pins"},
    {"ads1115", "ads1115 [status|probe|read [0-3]]", "ADS1115 probe/status and single-ended raw/mV read"},
    {"mcp23017", "mcp23017 [status|probe|scan|test|read <reg>|write <reg> <val>|pinmode <0-15> <in|out>|pinwrite <0-15> <0|1>|pinread <0-15>]", "MCP23017 I2C GPIO-expander diagnostics"},
    {"tp4056", "tp4056 [status|on|off]", "TP4056 charger status and optional CE control"},
    {"mcp2544fd", "mcp2544fd [status|on|off|standby]", "MCP2544FD CAN transceiver control pins"},
    {"mcp2515", "mcp2515 [status|probe|reset|read <reg>|write <reg> <val>|tx <id> <hex>|rx]", "MCP2515 SPI/CAN diagnostics (reg + tx/rx bring-up)"},
    {"uln2003", "uln2003 [status|off|test|step <steps> [delay_ms]]", "ULN2003 stepper driver control"},
    {"l298n", "l298n [status|stop|test|speed <0-100>|a <fwd|rev|stop|speed <0-100>>|b <fwd|rev|stop|speed <0-100>>]", "L298N dual H-bridge motor control"},
    {"wifi", "wifi [status|scan|connect|reconnect|disconnect]", "Wi-Fi station tools"},
    {"bluetooth", "bluetooth [status|on|off|scan [seconds]]", "Bluetooth diagnostics and BLE scan tools"},
    {"can", "can [status|up|down|send <id> <hex>|recv [timeout_ms]]", "TWAI/CAN diagnostics and frame TX/RX"},
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

static void bsh_print_usage(const char *cmd) {
    const bsh_cmd_help_t *h = bsh_find_help(cmd);
    if (h && h->usage) {
        basalt_printf("usage: %s\n", h->usage);
        return;
    }
    basalt_printf("usage: help -commands\n");
}

static void bsh_print_unknown_subcommand(const char *cmd, const char *sub) {
    basalt_printf("%s: unknown subcommand '%s'\n", cmd ? cmd : "command", sub ? sub : "");
    bsh_print_usage(cmd);
}

static void bsh_print_unknown_command(const char *cmd) {
    basalt_printf("unknown command: %s\n", cmd ? cmd : "");
    basalt_printf("hint: use 'help -commands'\n");
}

#if BASALT_SHELL_LEVEL == 1
static bool bsh_help_cmd_hidden_tiny(const char *name) {
    static const char *k_hidden[] = {
        "ls", "cat", "cd", "mkdir", "cp", "mv", "rm",
        "apps_dev", "led_test", "devcheck", "edit",
        "run_dev", "kill", "applet",
        "install", "remove", "logs", "imu", "dht22", "bme280", "ads1115", "mcp23017", "tp4056", "mcp2544fd", "mcp2515", "uln2003", "l298n", "wifi", "bluetooth", "can"
    };
    for (size_t i = 0; i < sizeof(k_hidden) / sizeof(k_hidden[0]); ++i) {
        if (strcmp(name, k_hidden[i]) == 0) return true;
    }
    return false;
}
#endif

static char g_cwd[64] = "/";
static char g_prev_cwd[64] = "/";

static void bsh_print_help(bool verbose) {
    if (!verbose) {
        basalt_printf("Basalt shell. Type 'help -commands' for the list, or 'help <cmd>' for details.\n");
        return;
    }
    const char *level = "off";
#if BASALT_SHELL_LEVEL >= 3
    level = "full";
#elif BASALT_SHELL_LEVEL == 2
    level = "lite";
#elif BASALT_SHELL_LEVEL == 1
    level = "tiny";
#endif
    basalt_printf("Shell level: %s\n\n", level);
    for (size_t i = 0; i < sizeof(k_bsh_help) / sizeof(k_bsh_help[0]); ++i) {
#if BASALT_SHELL_LEVEL == 1
        if (bsh_help_cmd_hidden_tiny(k_bsh_help[i].name)) continue;
#endif
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

static bool resolve_named_app_script(const char *app_name, char *out, size_t out_len) {
    if (!app_name || !app_name[0] || strchr(app_name, '/')) return false;

    char app_prefix[128];
    snprintf(app_prefix, sizeof(app_prefix), BASALT_APPS_ROOT "/%s", app_name);

    char script[256];
    app_entry_path_flat(app_prefix, script, sizeof(script));
    if (path_is_file(script)) {
        snprintf(out, out_len, "%s", script);
        return true;
    }

    snprintf(script, sizeof(script), "%s/main.py", app_prefix);
    if (path_is_file(script)) {
        snprintf(out, out_len, "%s", script);
        return true;
    }

    // SPIFFS can flatten paths; scan for "<app>/main.py" or any "<app>/*.py" fallback.
    DIR *dir = opendir(BASALT_APPS_ROOT);
    if (!dir) return false;

    char prefix[96];
    snprintf(prefix, sizeof(prefix), "%s/", app_name);
    size_t prefix_len = strlen(prefix);
    char first_py[256] = {0};
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (strncmp(name, prefix, prefix_len) != 0) continue;
        size_t nlen = strlen(name);
        if (nlen < 3 || strcmp(name + (nlen - 3), ".py") != 0) continue;

        char full[256];
        int full_len = snprintf(full, sizeof(full), BASALT_APPS_ROOT "/%s", name);
        if (full_len < 0 || full_len >= (int)sizeof(full)) continue;
        if (!path_is_file(full)) continue;

        if (strcmp(name + prefix_len, "main.py") == 0) {
            snprintf(out, out_len, "%s", full);
            closedir(dir);
            return true;
        }
        if (first_py[0] == '\0') {
            snprintf(first_py, sizeof(first_py), "%s", full);
        }
    }
    closedir(dir);

    if (first_py[0]) {
        snprintf(out, out_len, "%s", first_py);
        return true;
    }
    return false;
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

#if BASALT_SHELL_LEVEL >= 3
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

#if BASALT_SHELL_LEVEL >= 3
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

static void bsh_cmd_drivers(void);

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
        basalt_printf("devcheck: driver inventory\n");
        bsh_cmd_drivers();
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

static const char *bsh_enabled_str(int enabled) {
    return enabled ? "enabled" : "disabled";
}

static void bsh_cmd_drivers(void) {
    basalt_printf("drivers.core:\n");
    basalt_printf("  gpio: %s\n", bsh_enabled_str(BASALT_ENABLE_GPIO));
    basalt_printf("  i2c: %s\n", bsh_enabled_str(BASALT_ENABLE_I2C));
    basalt_printf("  spi: %s\n", bsh_enabled_str(BASALT_ENABLE_SPI));
    basalt_printf("  adc: %s\n", bsh_enabled_str(BASALT_ENABLE_ADC));
    basalt_printf("  tft: %s (%s)\n", bsh_enabled_str(BASALT_ENABLE_TFT), tft_console_is_ready() ? "runtime ready" : "runtime not ready");
    basalt_printf("  wifi: %s\n", bsh_enabled_str(BASALT_ENABLE_WIFI));
    basalt_printf("  bluetooth: %s\n", bsh_enabled_str(BASALT_ENABLE_BLUETOOTH));
    basalt_printf("  twai: %s\n", bsh_enabled_str(BASALT_ENABLE_TWAI));

    basalt_printf("drivers.experimental:\n");
#if BASALT_ENABLE_DISPLAY_SSD1306
    basalt_printf("  display_ssd1306: enabled (MicroPython API: basalt.ssd1306.*)\n");
#else
    basalt_printf("  display_ssd1306: disabled\n");
#endif
#if BASALT_ENABLE_RTC
    basalt_printf("  rtc: enabled (MicroPython API: basalt.rtc.available()/now())\n");
#else
    basalt_printf("  rtc: disabled\n");
#endif
#if BASALT_ENABLE_IMU
    basalt_printf("  imu: enabled (shell API: imu status/read/whoami)\n");
#else
    basalt_printf("  imu: disabled\n");
#endif
#if BASALT_ENABLE_DHT22
    basalt_printf("  dht22: enabled (shell API: dht22 status/read [pin] [auto|dht22|dht11])\n");
    basalt_printf("    pin: GPIO%d\n", BASALT_PIN_DHT22_DATA);
    basalt_printf("    sample_ms_cfg: %ld\n", (long)BASALT_CFG_DHT22_SAMPLE_MS);
    basalt_printf("    sensor_type_cfg: %s\n", BASALT_CFG_DHT_SENSOR_TYPE);
#else
    basalt_printf("  dht22: disabled\n");
#endif
#if BASALT_ENABLE_BME280
    basalt_printf("  bme280: enabled (shell API: bme280 status/probe/read)\n");
#else
    basalt_printf("  bme280: disabled\n");
#endif
#if BASALT_ENABLE_ADS1115
    basalt_printf("  ads1115: enabled (shell API: ads1115 status/probe/read)\n");
#else
    basalt_printf("  ads1115: disabled\n");
#endif
#if BASALT_ENABLE_MCP23017
    basalt_printf("  mcp23017: enabled (shell API: mcp23017 status/probe/scan/test/read/write/pinmode/pinwrite/pinread)\n");
#else
    basalt_printf("  mcp23017: disabled\n");
#endif
#if BASALT_ENABLE_MIC
    basalt_printf("  mic: enabled (stub only; runtime driver not implemented yet)\n");
#ifdef BASALT_PIN_MIC_IN
    basalt_printf("    pin: GPIO%d\n", BASALT_PIN_MIC_IN);
#else
    basalt_printf("    pin: not bound (define BASALT_PIN_MIC_IN in board config)\n");
#endif
#else
    basalt_printf("  mic: disabled\n");
#endif
#if BASALT_ENABLE_TP4056
    basalt_printf("  tp4056: enabled (shell API: tp4056 status/on/off)\n");
#else
    basalt_printf("  tp4056: disabled\n");
#endif
#if BASALT_ENABLE_MCP2544FD
    basalt_printf("  mcp2544fd: enabled (shell API: mcp2544fd status/on/off/standby)\n");
#else
    basalt_printf("  mcp2544fd: disabled\n");
#endif
#if BASALT_ENABLE_MCP2515
    basalt_printf("  mcp2515: enabled (shell API: mcp2515 status/probe/reset/read/write/tx/rx)\n");
#else
    basalt_printf("  mcp2515: disabled\n");
#endif
#if BASALT_ENABLE_ULN2003
    basalt_printf("  uln2003: enabled (shell API: uln2003 status/off/step)\n");
#else
    basalt_printf("  uln2003: disabled\n");
#endif
#if BASALT_ENABLE_L298N
    basalt_printf("  l298n: enabled (shell API: l298n status/stop/test/speed/a/b)\n");
#else
    basalt_printf("  l298n: disabled\n");
#endif
    basalt_printf("hint: these are configuration gates today; runtime implementations are incremental.\n");
}

#if BASALT_ENABLE_MCP2515
static bool s_mcp2515_stub_ready = false;
static bool s_mcp2515_spi_ready = false;
static spi_device_handle_t s_mcp2515_dev = NULL;

#define MCP2515_CMD_RESET          0xC0
#define MCP2515_CMD_READ           0x03
#define MCP2515_CMD_WRITE          0x02
#define MCP2515_CMD_RTS_TXB0       0x81
#define MCP2515_CMD_READ_RX_STATUS 0xB0

#define MCP2515_REG_CANSTAT        0x0E
#define MCP2515_REG_CANINTF        0x2C
#define MCP2515_REG_TXB0CTRL       0x30
#define MCP2515_REG_TXB0SIDH       0x31
#define MCP2515_REG_TXB0SIDL       0x32
#define MCP2515_REG_TXB0DLC        0x35
#define MCP2515_REG_TXB0D0         0x36
#define MCP2515_REG_RXB0DLC        0x65
#define MCP2515_REG_RXB0D0         0x66

#define MCP2515_CANINTF_RX0IF      0x01
#define MCP2515_CANINTF_RX1IF      0x02

static bool bsh_mcp2515_stub_init(char *err, size_t err_len) {
    if (s_mcp2515_stub_ready) return true;
#if !BASALT_ENABLE_SPI
    if (err && err_len) snprintf(err, err_len, "spi driver not enabled");
    return false;
#else
    if (BASALT_PIN_CAN_CS < 0 || BASALT_PIN_CAN_INT < 0) {
        if (err && err_len) snprintf(err, err_len, "missing MCP2515 pins (can_cs=%d can_int=%d)", BASALT_PIN_CAN_CS, BASALT_PIN_CAN_INT);
        return false;
    }
    esp_err_t ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_CAN_CS);
    if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_CAN_CS, GPIO_MODE_OUTPUT);
    if (ret == ESP_OK) ret = gpio_set_level((gpio_num_t)BASALT_PIN_CAN_CS, 1);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "can_cs init failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_CAN_INT);
    if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_CAN_INT, GPIO_MODE_INPUT);
    if (ret == ESP_OK) ret = gpio_set_pull_mode((gpio_num_t)BASALT_PIN_CAN_INT, GPIO_PULLUP_ONLY);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "can_int init failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_mcp2515_stub_ready = true;
    return true;
#endif
}

static bool bsh_mcp2515_spi_ensure(char *err, size_t err_len) {
    if (s_mcp2515_spi_ready && s_mcp2515_dev) return true;
    if (!bsh_mcp2515_stub_init(err, err_len)) return false;
    if (BASALT_PIN_SPI_SCLK < 0 || BASALT_PIN_SPI_MISO < 0 || BASALT_PIN_SPI_MOSI < 0) {
        if (err && err_len) snprintf(err, err_len, "missing SPI pins (sclk=%d miso=%d mosi=%d)", BASALT_PIN_SPI_SCLK, BASALT_PIN_SPI_MISO, BASALT_PIN_SPI_MOSI);
        return false;
    }

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = BASALT_PIN_SPI_MOSI,
        .miso_io_num = BASALT_PIN_SPI_MISO,
        .sclk_io_num = BASALT_PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) snprintf(err, err_len, "spi bus init failed (%s)", esp_err_to_name(ret));
        return false;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = BASALT_PIN_CAN_CS,
        .queue_size = 1,
    };
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_mcp2515_dev);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "spi add device failed (%s)", esp_err_to_name(ret));
        return false;
    }

    s_mcp2515_spi_ready = true;
    return true;
}

static bool bsh_mcp2515_spi_xfer(const uint8_t *tx, uint8_t *rx, size_t len, char *err, size_t err_len) {
    if (!s_mcp2515_spi_ready || !s_mcp2515_dev) {
        if (err && err_len) snprintf(err, err_len, "spi device not ready");
        return false;
    }
    if (!tx || len == 0) {
        if (err && err_len) snprintf(err, err_len, "invalid spi xfer args");
        return false;
    }

    spi_transaction_t t = {
        .length = (uint32_t)(len * 8),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t ret = spi_device_transmit(s_mcp2515_dev, &t);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "spi xfer failed (%s)", esp_err_to_name(ret));
        return false;
    }
    return true;
}

static bool bsh_mcp2515_reg_read(uint8_t reg, uint8_t *val, char *err, size_t err_len) {
    uint8_t tx[3] = {MCP2515_CMD_READ, reg, 0x00};
    uint8_t rx[3] = {0};
    if (!bsh_mcp2515_spi_xfer(tx, rx, sizeof(tx), err, err_len)) return false;
    if (val) *val = rx[2];
    return true;
}

static bool bsh_mcp2515_reg_write(uint8_t reg, uint8_t val, char *err, size_t err_len) {
    uint8_t tx[3] = {MCP2515_CMD_WRITE, reg, val};
    return bsh_mcp2515_spi_xfer(tx, NULL, sizeof(tx), err, err_len);
}

static bool bsh_parse_u8(const char *txt, uint8_t *out) {
    if (!txt || !out) return false;
    char *end = NULL;
    long v = strtol(txt, &end, 0);
    if (end == txt || (end && *end != '\0') || v < 0 || v > 0xFF) return false;
    *out = (uint8_t)v;
    return true;
}

static bool bsh_parse_u16(const char *txt, uint16_t *out) {
    if (!txt || !out) return false;
    char *end = NULL;
    long v = strtol(txt, &end, 0);
    if (end == txt || (end && *end != '\0') || v < 0 || v > 0xFFFF) return false;
    *out = (uint16_t)v;
    return true;
}

static int bsh_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool bsh_parse_hex_bytes(const char *hex, uint8_t *out, uint8_t *out_len) {
    if (!hex || !out || !out_len) return false;
    size_t n = strlen(hex);
    if (n == 0 || (n % 2) != 0 || n > 16) return false;
    uint8_t len = (uint8_t)(n / 2);
    for (uint8_t i = 0; i < len; ++i) {
        int hi = bsh_hex_nibble(hex[i * 2]);
        int lo = bsh_hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    *out_len = len;
    return true;
}

static bool bsh_mcp2515_send_std(uint16_t sid, const uint8_t *data, uint8_t dlc, char *err, size_t err_len) {
    if (sid > 0x7FF) {
        if (err && err_len) snprintf(err, err_len, "standard id out of range (0x%03X)", sid);
        return false;
    }
    if (dlc > 8) {
        if (err && err_len) snprintf(err, err_len, "invalid dlc %u", (unsigned)dlc);
        return false;
    }

    uint8_t sidh = (uint8_t)(sid >> 3);
    uint8_t sidl = (uint8_t)((sid & 0x7) << 5);
    if (!bsh_mcp2515_reg_write(MCP2515_REG_TXB0SIDH, sidh, err, err_len)) return false;
    if (!bsh_mcp2515_reg_write(MCP2515_REG_TXB0SIDL, sidl, err, err_len)) return false;
    if (!bsh_mcp2515_reg_write(MCP2515_REG_TXB0DLC, (uint8_t)(dlc & 0x0F), err, err_len)) return false;
    for (uint8_t i = 0; i < dlc; ++i) {
        if (!bsh_mcp2515_reg_write((uint8_t)(MCP2515_REG_TXB0D0 + i), data[i], err, err_len)) return false;
    }

    uint8_t rts[1] = {MCP2515_CMD_RTS_TXB0};
    if (!bsh_mcp2515_spi_xfer(rts, NULL, sizeof(rts), err, err_len)) return false;
    return true;
}

static void bsh_cmd_mcp2515(const char *sub, const char *arg1, const char *arg2) {
    if (!sub || strcmp(sub, "status") == 0) {
        basalt_printf("mcp2515.enabled: yes\n");
        basalt_printf("mcp2515.spi_required: %s\n", BASALT_ENABLE_SPI ? "yes" : "no");
        basalt_printf("mcp2515.pins: can_cs=%d can_int=%d\n", BASALT_PIN_CAN_CS, BASALT_PIN_CAN_INT);
        basalt_printf("mcp2515.spi_pins: sclk=%d miso=%d mosi=%d\n", BASALT_PIN_SPI_SCLK, BASALT_PIN_SPI_MISO, BASALT_PIN_SPI_MOSI);
        basalt_printf("mcp2515.stub_ready: %s\n", s_mcp2515_stub_ready ? "yes" : "no");
        basalt_printf("mcp2515.spi_ready: %s\n", (s_mcp2515_spi_ready && s_mcp2515_dev) ? "yes" : "no");
        basalt_printf("mcp2515.note: probe/read/write/tx/rx are bring-up diagnostics; full CAN workflow requires hardware validation.\n");
        return;
    }

    char err[96];
    if (!bsh_mcp2515_spi_ensure(err, sizeof(err))) {
        basalt_printf("mcp2515: %s\n", err);
        return;
    }

    if (strcmp(sub, "reset") == 0) {
        uint8_t reset_cmd[1] = {MCP2515_CMD_RESET};
        if (!bsh_mcp2515_spi_xfer(reset_cmd, NULL, sizeof(reset_cmd), err, sizeof(err))) {
            basalt_printf("mcp2515 reset: fail (%s)\n", err);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        basalt_printf("mcp2515 reset: ok\n");
        return;
    }

    if (strcmp(sub, "read") == 0) {
        uint8_t reg = 0;
        if (!bsh_parse_u8(arg1, &reg)) {
            basalt_printf("mcp2515 read: invalid reg '%s' (expected 0..255 or 0x00..0xFF)\n", arg1 ? arg1 : "");
            return;
        }
        uint8_t val = 0;
        if (!bsh_mcp2515_reg_read(reg, &val, err, sizeof(err))) {
            basalt_printf("mcp2515 read: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp2515 read: reg=0x%02X val=0x%02X\n", reg, val);
        return;
    }

    if (strcmp(sub, "write") == 0) {
        uint8_t reg = 0;
        uint8_t val = 0;
        if (!bsh_parse_u8(arg1, &reg) || !bsh_parse_u8(arg2, &val)) {
            basalt_printf("mcp2515 write: invalid args (usage: mcp2515 write <reg> <val>)\n");
            return;
        }
        if (!bsh_mcp2515_reg_write(reg, val, err, sizeof(err))) {
            basalt_printf("mcp2515 write: fail (%s)\n", err);
            return;
        }
        uint8_t verify = 0;
        if (!bsh_mcp2515_reg_read(reg, &verify, err, sizeof(err))) {
            basalt_printf("mcp2515 write: wrote 0x%02X but verify read failed (%s)\n", val, err);
            return;
        }
        basalt_printf("mcp2515 write: reg=0x%02X val=0x%02X verify=0x%02X\n", reg, val, verify);
        return;
    }

    if (strcmp(sub, "tx") == 0) {
        uint16_t sid = 0;
        if (!bsh_parse_u16(arg1, &sid) || sid > 0x7FF) {
            basalt_printf("mcp2515 tx: invalid id '%s' (expected 0..0x7FF)\n", arg1 ? arg1 : "");
            return;
        }
        uint8_t data[8] = {0};
        uint8_t dlc = 0;
        if (!bsh_parse_hex_bytes(arg2, data, &dlc)) {
            basalt_printf("mcp2515 tx: invalid data '%s' (expected 1..8 bytes as hex pairs)\n", arg2 ? arg2 : "");
            return;
        }
        if (!bsh_mcp2515_send_std(sid, data, dlc, err, sizeof(err))) {
            basalt_printf("mcp2515 tx: fail (%s)\n", err);
            return;
        }
        uint8_t txb0ctrl = 0;
        if (!bsh_mcp2515_reg_read(MCP2515_REG_TXB0CTRL, &txb0ctrl, err, sizeof(err))) {
            basalt_printf("mcp2515 tx: sent id=0x%03X dlc=%u (status read failed: %s)\n", sid, (unsigned)dlc, err);
            return;
        }
        basalt_printf("mcp2515 tx: id=0x%03X dlc=%u txb0ctrl=0x%02X\n", sid, (unsigned)dlc, txb0ctrl);
        return;
    }

    if (strcmp(sub, "rx") == 0) {
        uint8_t rxst_tx[2] = {MCP2515_CMD_READ_RX_STATUS, 0x00};
        uint8_t rxst_rx[2] = {0};
        if (!bsh_mcp2515_spi_xfer(rxst_tx, rxst_rx, sizeof(rxst_tx), err, sizeof(err))) {
            basalt_printf("mcp2515 rx: fail (%s)\n", err);
            return;
        }

        uint8_t canintf = 0;
        if (!bsh_mcp2515_reg_read(MCP2515_REG_CANINTF, &canintf, err, sizeof(err))) {
            basalt_printf("mcp2515 rx: fail (%s)\n", err);
            return;
        }

        basalt_printf("mcp2515 rx: rx_status=0x%02X canintf=0x%02X rx0if=%s rx1if=%s\n",
                      rxst_rx[1], canintf,
                      (canintf & MCP2515_CANINTF_RX0IF) ? "yes" : "no",
                      (canintf & MCP2515_CANINTF_RX1IF) ? "yes" : "no");

        if (canintf & MCP2515_CANINTF_RX0IF) {
            uint8_t dlc = 0;
            if (!bsh_mcp2515_reg_read(MCP2515_REG_RXB0DLC, &dlc, err, sizeof(err))) {
                basalt_printf("mcp2515 rx: rx0 dlc read failed (%s)\n", err);
                return;
            }
            uint8_t n = (uint8_t)(dlc & 0x0F);
            if (n > 8) n = 8;
            basalt_printf("mcp2515 rx0: dlc=%u data=", (unsigned)n);
            for (uint8_t i = 0; i < n; ++i) {
                uint8_t b = 0;
                if (!bsh_mcp2515_reg_read((uint8_t)(MCP2515_REG_RXB0D0 + i), &b, err, sizeof(err))) {
                    basalt_printf("<err:%s>", err);
                    break;
                }
                basalt_printf("%02X", b);
            }
            basalt_printf("\n");
        }
        return;
    }

    if (strcmp(sub, "probe") == 0) {
        uint8_t reset_cmd[1] = {MCP2515_CMD_RESET};
        if (!bsh_mcp2515_spi_xfer(reset_cmd, NULL, sizeof(reset_cmd), err, sizeof(err))) {
            basalt_printf("mcp2515 probe: fail (%s)\n", err);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(5));

        uint8_t canstat = 0;
        if (!bsh_mcp2515_reg_read(MCP2515_REG_CANSTAT, &canstat, err, sizeof(err))) {
            basalt_printf("mcp2515 probe: fail (%s)\n", err);
            return;
        }

        uint8_t before = 0;
        if (!bsh_mcp2515_reg_read(MCP2515_REG_TXB0D0, &before, err, sizeof(err))) {
            basalt_printf("mcp2515 probe: fail (%s)\n", err);
            return;
        }
        uint8_t pattern = (uint8_t)(before ^ 0x5A);
        if (!bsh_mcp2515_reg_write(MCP2515_REG_TXB0D0, pattern, err, sizeof(err))) {
            basalt_printf("mcp2515 probe: fail (%s)\n", err);
            return;
        }
        uint8_t after = 0;
        if (!bsh_mcp2515_reg_read(MCP2515_REG_TXB0D0, &after, err, sizeof(err))) {
            basalt_printf("mcp2515 probe: fail (%s)\n", err);
            return;
        }
        (void)bsh_mcp2515_reg_write(MCP2515_REG_TXB0D0, before, NULL, 0);

        if (after != pattern) {
            basalt_printf("mcp2515 probe: fail (rw mismatch reg=0x%02X wrote=0x%02X read=0x%02X)\n", MCP2515_REG_TXB0D0, pattern, after);
            return;
        }
        basalt_printf("mcp2515 probe: ok (CANSTAT=0x%02X TXB0D0 rw=ok)\n", canstat);
        return;
    }

    basalt_printf("usage: mcp2515 [status|probe|reset|read <reg>|write <reg> <val>|tx <id> <hex>|rx]\n");
}

#else
static void bsh_cmd_mcp2515(const char *sub, const char *arg1, const char *arg2) {
    (void)sub;
    (void)arg1;
    (void)arg2;
    basalt_printf("mcp2515: unavailable (enable mcp2515+spi drivers)\n");
}
#endif

#if BASALT_SHELL_LEVEL >= 3
#if BASALT_ENABLE_IMU && BASALT_ENABLE_I2C
static bool s_imu_i2c_ready = false;
static bool s_imu_configured = false;

static uint8_t bsh_imu_addr(void) {
    long v = strtol(BASALT_CFG_IMU_ADDRESS, NULL, 0);
    if (v <= 0 || v > 0x7F) v = 0x68;
    return (uint8_t)v;
}

static const char *bsh_imu_name_from_id(uint8_t who) {
    switch (who) {
        case 0x19: return "MPU6886";
        case 0x68: return "MPU6050";
        case 0x71: return "MPU9250";
        case 0xEA: return "ICM20948";
        default: return "unknown";
    }
}

static bool bsh_imu_i2c_ensure(char *err, size_t err_len) {
    if (s_imu_i2c_ready) return true;
    if (BASALT_PIN_I2C_SDA < 0 || BASALT_PIN_I2C_SCL < 0) {
        if (err && err_len) snprintf(err, err_len, "invalid I2C pins (sda=%d scl=%d)", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
        return false;
    }
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BASALT_PIN_I2C_SDA,
        .scl_io_num = BASALT_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_param_config failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        s_imu_i2c_ready = true;
        return true;
    }
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_driver_install failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_imu_i2c_ready = true;
    return true;
}

static esp_err_t bsh_imu_read_regs(uint8_t reg, uint8_t *buf, size_t len) {
    const uint8_t addr = bsh_imu_addr();
    return i2c_master_write_read_device(I2C_NUM_0, addr, &reg, 1, buf, len, pdMS_TO_TICKS(60));
}

static esp_err_t bsh_imu_write_reg(uint8_t reg, uint8_t val) {
    const uint8_t addr = bsh_imu_addr();
    uint8_t payload[2] = {reg, val};
    return i2c_master_write_to_device(I2C_NUM_0, addr, payload, sizeof(payload), pdMS_TO_TICKS(60));
}

static bool bsh_imu_configure_if_needed(uint8_t who, char *err, size_t err_len) {
    if (s_imu_configured) return true;
    // Common wake + basic range setup (MPU family including MPU6886).
    esp_err_t ret = bsh_imu_write_reg(0x6B, 0x00); // PWR_MGMT_1: wake up
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "write PWR_MGMT_1 failed (%s)", esp_err_to_name(ret));
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    // Accelerometer range: +/-2g
    ret = bsh_imu_write_reg(0x1C, 0x00);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "write ACCEL_CONFIG failed (%s)", esp_err_to_name(ret));
        return false;
    }
    // Gyro range: keep per-sensor-friendly default.
    // For MPU6886 we use +/-2000 dps (0x18); for older MPU6050 keep default +/-250 dps.
    ret = bsh_imu_write_reg(0x1B, (who == 0x68) ? 0x00 : 0x18);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "write GYRO_CONFIG failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_imu_configured = true;
    return true;
}

static bool bsh_imu_probe(uint8_t *out_who, char *err, size_t err_len) {
    if (!bsh_imu_i2c_ensure(err, err_len)) return false;
    uint8_t who = 0;
    esp_err_t ret = bsh_imu_read_regs(0x75, &who, 1);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "WHO_AM_I read failed (%s)", esp_err_to_name(ret));
        return false;
    }
    if (out_who) *out_who = who;
    if (!bsh_imu_configure_if_needed(who, err, err_len)) return false;
    return true;
}

static void bsh_cmd_imu_status(void) {
    basalt_printf("imu.enabled: yes\n");
    basalt_printf("imu.driver_cfg: %s\n", BASALT_CFG_IMU_DRIVER);
    basalt_printf("imu.addr_cfg: %s\n", BASALT_CFG_IMU_ADDRESS);
    basalt_printf("imu.i2c_pins: sda=%d scl=%d\n", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
    uint8_t who = 0;
    char err[96];
    if (!bsh_imu_probe(&who, err, sizeof(err))) {
        basalt_printf("imu.probe: fail (%s)\n", err);
        return;
    }
    basalt_printf("imu.probe: ok\n");
    basalt_printf("imu.whoami: 0x%02X (%s)\n", who, bsh_imu_name_from_id(who));
}

static void bsh_cmd_imu_whoami(void) {
    uint8_t who = 0;
    char err[96];
    if (!bsh_imu_probe(&who, err, sizeof(err))) {
        basalt_printf("imu whoami: %s\n", err);
        return;
    }
    basalt_printf("imu whoami: 0x%02X (%s)\n", who, bsh_imu_name_from_id(who));
}

static void bsh_cmd_imu_read(void) {
    uint8_t who = 0;
    char err[96];
    if (!bsh_imu_probe(&who, err, sizeof(err))) {
        basalt_printf("imu read: %s\n", err);
        return;
    }
    uint8_t d[14] = {0};
    esp_err_t ret = bsh_imu_read_regs(0x3B, d, sizeof(d));
    if (ret != ESP_OK) {
        basalt_printf("imu read: data read failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    int16_t ax = (int16_t)((d[0] << 8) | d[1]);
    int16_t ay = (int16_t)((d[2] << 8) | d[3]);
    int16_t az = (int16_t)((d[4] << 8) | d[5]);
    int16_t tr = (int16_t)((d[6] << 8) | d[7]);
    int16_t gx = (int16_t)((d[8] << 8) | d[9]);
    int16_t gy = (int16_t)((d[10] << 8) | d[11]);
    int16_t gz = (int16_t)((d[12] << 8) | d[13]);

    float accel_lsb = 16384.0f; // +/-2g typical startup range
    float gyro_lsb = (who == 0x68) ? 131.0f : 16.4f; // MPU6050 default vs MPU6886/ICM typical high range
    float temp_c = (who == 0x68) ? ((float)tr / 340.0f + 36.53f) : ((float)tr / 326.8f + 25.0f);

    basalt_printf("imu.id: 0x%02X (%s)\n", who, bsh_imu_name_from_id(who));
    basalt_printf("imu.accel.raw: x=%d y=%d z=%d\n", ax, ay, az);
    basalt_printf("imu.accel.g: x=%.3f y=%.3f z=%.3f\n", ax / accel_lsb, ay / accel_lsb, az / accel_lsb);
    basalt_printf("imu.gyro.raw: x=%d y=%d z=%d\n", gx, gy, gz);
    basalt_printf("imu.gyro.dps: x=%.3f y=%.3f z=%.3f\n", gx / gyro_lsb, gy / gyro_lsb, gz / gyro_lsb);
    basalt_printf("imu.temp_c: %.2f\n", temp_c);
}

static void bsh_cmd_imu_stream(const char *hz_arg, const char *samples_arg) {
    int hz = 5;
    int samples = 50;
    if (hz_arg && hz_arg[0]) {
        hz = (int)strtol(hz_arg, NULL, 10);
    }
    if (samples_arg && samples_arg[0]) {
        samples = (int)strtol(samples_arg, NULL, 10);
    }
    if (hz < 1) hz = 1;
    if (hz > 50) hz = 50;
    if (samples < 1) samples = 1;
    if (samples > 1000) samples = 1000;

    int period_ms = 1000 / hz;
    if (period_ms < 10) period_ms = 10;

    uint8_t who = 0;
    char err[96];
    if (!bsh_imu_probe(&who, err, sizeof(err))) {
        basalt_printf("imu stream: %s\n", err);
        return;
    }
    float accel_lsb = 16384.0f;
    float gyro_lsb = (who == 0x68) ? 131.0f : 16.4f;

    basalt_printf("imu stream: id=0x%02X (%s), hz=%d, samples=%d\n", who, bsh_imu_name_from_id(who), hz, samples);
    basalt_printf("imu stream: idx ax_g ay_g az_g gx_dps gy_dps gz_dps temp_c\n");

    for (int i = 0; i < samples; ++i) {
        uint8_t d[14] = {0};
        esp_err_t ret = bsh_imu_read_regs(0x3B, d, sizeof(d));
        if (ret != ESP_OK) {
            basalt_printf("imu stream: read failed at sample %d (%s)\n", i + 1, esp_err_to_name(ret));
            return;
        }
        int16_t ax = (int16_t)((d[0] << 8) | d[1]);
        int16_t ay = (int16_t)((d[2] << 8) | d[3]);
        int16_t az = (int16_t)((d[4] << 8) | d[5]);
        int16_t tr = (int16_t)((d[6] << 8) | d[7]);
        int16_t gx = (int16_t)((d[8] << 8) | d[9]);
        int16_t gy = (int16_t)((d[10] << 8) | d[11]);
        int16_t gz = (int16_t)((d[12] << 8) | d[13]);
        float temp_c = (who == 0x68) ? ((float)tr / 340.0f + 36.53f) : ((float)tr / 326.8f + 25.0f);

        basalt_printf("imu %03d %.3f %.3f %.3f %.3f %.3f %.3f %.2f\n",
            i + 1,
            ax / accel_lsb, ay / accel_lsb, az / accel_lsb,
            gx / gyro_lsb, gy / gyro_lsb, gz / gyro_lsb,
            temp_c);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
    basalt_printf("imu stream: done\n");
}
#else
static void bsh_cmd_imu_status(void) {
    basalt_printf("imu: unavailable (enable imu+i2c drivers)\n");
}
static void bsh_cmd_imu_whoami(void) {
    basalt_printf("imu: unavailable (enable imu+i2c drivers)\n");
}
static void bsh_cmd_imu_read(void) {
    basalt_printf("imu: unavailable (enable imu+i2c drivers)\n");
}
static void bsh_cmd_imu_stream(const char *hz_arg, const char *samples_arg) {
    (void)hz_arg;
    (void)samples_arg;
    basalt_printf("imu: unavailable (enable imu+i2c drivers)\n");
}
#endif

#if BASALT_ENABLE_DHT22 && BASALT_ENABLE_GPIO
static int64_t s_dht22_last_read_us = 0;
static float s_dht22_last_temp_c = 0.0f;
static float s_dht22_last_hum_pct = 0.0f;
static bool s_dht22_has_last = false;
static portMUX_TYPE s_dht22_lock = portMUX_INITIALIZER_UNLOCKED;

typedef enum {
    BSH_DHT_MODE_AUTO = 0,
    BSH_DHT_MODE_DHT22 = 1,
    BSH_DHT_MODE_DHT11 = 2,
} bsh_dht_mode_t;

static int bsh_dht22_pin_from_arg(const char *pin_arg, bool *ok) {
    if (ok) *ok = false;
    if (!pin_arg || !pin_arg[0]) {
        if (ok) *ok = true;
        return BASALT_PIN_DHT22_DATA;
    }
    char *end = NULL;
    long v = strtol(pin_arg, &end, 0);
    if (end == pin_arg || (end && *end != '\0') || v < 0 || v > 48) return -1;
    if (ok) *ok = true;
    return (int)v;
}

static bsh_dht_mode_t bsh_dht_mode_from_cfg(void) {
    if (strcmp(BASALT_CFG_DHT_SENSOR_TYPE, "dht22") == 0) return BSH_DHT_MODE_DHT22;
    if (strcmp(BASALT_CFG_DHT_SENSOR_TYPE, "dht11") == 0) return BSH_DHT_MODE_DHT11;
    return BSH_DHT_MODE_AUTO;
}

static bsh_dht_mode_t bsh_dht_mode_from_arg(const char *arg, bool *ok) {
    if (ok) *ok = false;
    if (!arg || !arg[0]) {
        if (ok) *ok = true;
        return bsh_dht_mode_from_cfg();
    }
    if (strcmp(arg, "auto") == 0) {
        if (ok) *ok = true;
        return BSH_DHT_MODE_AUTO;
    }
    if (strcmp(arg, "dht22") == 0) {
        if (ok) *ok = true;
        return BSH_DHT_MODE_DHT22;
    }
    if (strcmp(arg, "dht11") == 0) {
        if (ok) *ok = true;
        return BSH_DHT_MODE_DHT11;
    }
    return BSH_DHT_MODE_AUTO;
}

static const char *bsh_dht_mode_name(bsh_dht_mode_t mode) {
    if (mode == BSH_DHT_MODE_DHT22) return "dht22";
    if (mode == BSH_DHT_MODE_DHT11) return "dht11";
    return "auto";
}

static bool bsh_dht22_wait_level(int pin, int level, uint32_t timeout_us) {
    const int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < (int64_t)timeout_us) {
        if (gpio_get_level((gpio_num_t)pin) == level) return true;
    }
    return false;
}

static int32_t bsh_dht22_expect_pulse_us(int pin, int level, uint32_t timeout_us) {
    const int64_t start = esp_timer_get_time();
    while (gpio_get_level((gpio_num_t)pin) == level) {
        if ((esp_timer_get_time() - start) > (int64_t)timeout_us) return -1;
    }
    return (int32_t)(esp_timer_get_time() - start);
}

static bool bsh_dht22_read_raw(int pin, uint8_t out[5], char *err, size_t err_len) {
    if (!out) {
        if (err && err_len) snprintf(err, err_len, "invalid output buffer");
        return false;
    }
    if (pin < 0) {
        if (err && err_len) snprintf(err, err_len, "data pin is not configured");
        return false;
    }

    memset(out, 0, 5);

    esp_err_t ret = gpio_reset_pin((gpio_num_t)pin);
    if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
    if (ret == ESP_OK) ret = gpio_set_level((gpio_num_t)pin, 1);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "pin init failed (%s)", esp_err_to_name(ret));
        return false;
    }

    // Match Arduino DHT library style timing more closely:
    // keep line high briefly, then issue a longer start-low pulse.
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level((gpio_num_t)pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)pin, 1);
    esp_rom_delay_us(40);

    ret = gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
    if (ret == ESP_OK) ret = gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "pin input mode failed (%s)", esp_err_to_name(ret));
        return false;
    }

    int fail_stage = 0;
    int fail_bit = -1;
    portENTER_CRITICAL(&s_dht22_lock);
    if (!bsh_dht22_wait_level(pin, 0, 1000)) {
        fail_stage = 1;
    } else if (bsh_dht22_expect_pulse_us(pin, 0, 1000) < 0) {
        fail_stage = 2;
    } else if (bsh_dht22_expect_pulse_us(pin, 1, 1000) < 0) {
        fail_stage = 3;
    } else {
        for (int bit_idx = 0; bit_idx < 40; ++bit_idx) {
            int32_t low_us = bsh_dht22_expect_pulse_us(pin, 0, 1000);
            if (low_us < 0) {
                fail_stage = 4;
                fail_bit = bit_idx;
                break;
            }
            int32_t high_us = bsh_dht22_expect_pulse_us(pin, 1, 1000);
            if (high_us < 0) {
                fail_stage = 5;
                fail_bit = bit_idx;
                break;
            }
            const uint8_t bit = (high_us > low_us) ? 1u : 0u;
            out[bit_idx / 8] = (uint8_t)((out[bit_idx / 8] << 1) | bit);
        }
    }
    portEXIT_CRITICAL(&s_dht22_lock);

    if (fail_stage == 1) {
        if (err && err_len) snprintf(err, err_len, "timeout waiting for sensor response low");
        return false;
    }
    if (fail_stage == 2) {
        if (err && err_len) snprintf(err, err_len, "timeout during sensor response low pulse");
        return false;
    }
    if (fail_stage == 3) {
        if (err && err_len) snprintf(err, err_len, "timeout during sensor response high pulse");
        return false;
    }
    if (fail_stage == 4) {
        if (err && err_len) snprintf(err, err_len, "timeout during bit %d low pulse", fail_bit);
        return false;
    }
    if (fail_stage == 5) {
        if (err && err_len) snprintf(err, err_len, "timeout during bit %d high pulse", fail_bit);
        return false;
    }

    const uint8_t checksum = (uint8_t)(out[0] + out[1] + out[2] + out[3]);
    if (checksum != out[4]) {
        if (err && err_len) snprintf(err, err_len, "checksum mismatch (calc=0x%02X recv=0x%02X)", checksum, out[4]);
        return false;
    }
    return true;
}

static void bsh_dht22_decode(const uint8_t raw[5], float *temp_c, float *hum_pct) {
    if (!raw || !temp_c || !hum_pct) return;
    const uint16_t hum_raw = (uint16_t)(((uint16_t)raw[0] << 8) | (uint16_t)raw[1]);
    const uint16_t temp_raw_u = (uint16_t)(((uint16_t)raw[2] << 8) | (uint16_t)raw[3]);
    const bool neg = (temp_raw_u & 0x8000u) != 0;
    const uint16_t temp_mag = (uint16_t)(temp_raw_u & 0x7FFFu);

    *hum_pct = ((float)hum_raw) / 10.0f;
    *temp_c = ((float)temp_mag) / 10.0f;
    if (neg) *temp_c = -*temp_c;
}

static void bsh_dht11_decode(const uint8_t raw[5], float *temp_c, float *hum_pct) {
    if (!raw || !temp_c || !hum_pct) return;
    *hum_pct = (float)raw[0] + ((float)raw[1] / 10.0f);
    *temp_c = (float)raw[2] + ((float)raw[3] / 10.0f);
}

static bool bsh_dht_values_plausible(float temp_c, float hum_pct) {
    if (hum_pct < 0.0f || hum_pct > 100.0f) return false;
    if (temp_c < -40.0f || temp_c > 125.0f) return false;
    return true;
}

static bool bsh_dht11_frame_plausible(const uint8_t raw[5], float temp_c, float hum_pct) {
    if (!raw) return false;
    // DHT11 uses integer + single decimal digit bytes (commonly 0).
    if (raw[1] > 9 || raw[3] > 9) return false;
    if (hum_pct < 0.0f || hum_pct > 100.0f) return false;
    if (temp_c < 0.0f || temp_c > 80.0f) return false;
    return true;
}

static void bsh_cmd_dht22_status(void) {
    basalt_printf("dht22.enabled: yes\n");
    basalt_printf("dht22.pin: %d\n", BASALT_PIN_DHT22_DATA);
    basalt_printf("dht22.sample_ms_cfg: %ld\n", (long)BASALT_CFG_DHT22_SAMPLE_MS);
    basalt_printf("dht22.sensor_type_cfg: %s\n", BASALT_CFG_DHT_SENSOR_TYPE);
    basalt_printf("dht22.last_read_available: %s\n", s_dht22_has_last ? "yes" : "no");
    if (s_dht22_has_last) {
        const int64_t age_ms = (esp_timer_get_time() - s_dht22_last_read_us) / 1000;
        basalt_printf("dht22.last_read_ms_ago: %lld\n", (long long)age_ms);
        basalt_printf("dht22.last_temp_c: %.1f\n", (double)s_dht22_last_temp_c);
        basalt_printf("dht22.last_humidity_pct: %.1f\n", (double)s_dht22_last_hum_pct);
    }
    if (BASALT_PIN_DHT22_DATA < 0) {
        basalt_printf("dht22.status: pin not configured (set BASALT_PIN_DHT22_DATA)\n");
    }
}

static void bsh_cmd_dht22_read(const char *pin_arg, const char *mode_arg) {
    bool pin_ok = false;
    const int pin = bsh_dht22_pin_from_arg(pin_arg, &pin_ok);
    if (!pin_ok) {
        basalt_printf("dht22 read: invalid pin '%s'\n", pin_arg);
        return;
    }
    bool mode_ok = false;
    bsh_dht_mode_t mode = bsh_dht_mode_from_arg(mode_arg, &mode_ok);
    if (!mode_ok) {
        basalt_printf("dht22 read: invalid mode '%s' (use auto|dht22|dht11)\n", mode_arg);
        return;
    }

    const int64_t now = esp_timer_get_time();
    const int64_t min_interval_us = (int64_t)BASALT_CFG_DHT22_SAMPLE_MS * 1000;
    if (s_dht22_has_last && min_interval_us > 0) {
        const int64_t delta = now - s_dht22_last_read_us;
        if (delta > 0 && delta < min_interval_us) {
            const int64_t wait_ms = (min_interval_us - delta + 999) / 1000;
            basalt_printf("dht22 read: wait %lld ms (min interval %ld ms)\n", (long long)wait_ms, (long)BASALT_CFG_DHT22_SAMPLE_MS);
            return;
        }
    }

    uint8_t raw[5] = {0};
    char err[96];
    if (!bsh_dht22_read_raw(pin, raw, err, sizeof(err))) {
        basalt_printf("dht22 read: fail (%s)\n", err);
        return;
    }

    float temp_c = 0.0f;
    float hum_pct = 0.0f;
    float dht22_temp = 0.0f;
    float dht22_hum = 0.0f;
    float dht11_temp = 0.0f;
    float dht11_hum = 0.0f;
    bsh_dht22_decode(raw, &dht22_temp, &dht22_hum);
    bsh_dht11_decode(raw, &dht11_temp, &dht11_hum);

    const bool dht22_ok = bsh_dht_values_plausible(dht22_temp, dht22_hum);
    const bool dht11_ok = bsh_dht11_frame_plausible(raw, dht11_temp, dht11_hum);
    const char *detected = "dht22";
    if (mode == BSH_DHT_MODE_DHT22) {
        temp_c = dht22_temp;
        hum_pct = dht22_hum;
        detected = "dht22";
    } else if (mode == BSH_DHT_MODE_DHT11) {
        temp_c = dht11_temp;
        hum_pct = dht11_hum;
        detected = "dht11";
    } else {
        if (dht22_ok) {
            temp_c = dht22_temp;
            hum_pct = dht22_hum;
            detected = "dht22";
        } else {
            temp_c = dht11_temp;
            hum_pct = dht11_hum;
            detected = "dht11";
        }
    }

    s_dht22_last_temp_c = temp_c;
    s_dht22_last_hum_pct = hum_pct;
    s_dht22_last_read_us = esp_timer_get_time();
    s_dht22_has_last = true;

    basalt_printf("dht22 read: ok\n");
    basalt_printf("dht22.pin_used: %d\n", pin);
    basalt_printf("dht22.mode_used: %s\n", bsh_dht_mode_name(mode));
    basalt_printf("dht22.sensor_detected: %s\n", detected);
    basalt_printf("dht22.decode_valid.dht22: %s\n", dht22_ok ? "yes" : "no");
    basalt_printf("dht22.decode_valid.dht11: %s\n", dht11_ok ? "yes" : "no");
    basalt_printf("dht22.temp_c: %.1f\n", (double)temp_c);
    basalt_printf("dht22.humidity_pct: %.1f\n", (double)hum_pct);
    basalt_printf("dht22.raw: %02X %02X %02X %02X %02X\n",
        raw[0], raw[1], raw[2], raw[3], raw[4]);
}
#else
static void bsh_cmd_dht22_status(void) {
    basalt_printf("dht22: unavailable (enable dht22+gpio drivers)\n");
}
static void bsh_cmd_dht22_read(const char *pin_arg, const char *mode_arg) {
    (void)pin_arg;
    (void)mode_arg;
    basalt_printf("dht22 read: unavailable (enable dht22+gpio drivers)\n");
}
#endif


#if BASALT_ENABLE_BME280 && BASALT_ENABLE_I2C
static bool s_bme280_i2c_ready = false;

static uint8_t bsh_bme280_addr(void) {
    long v = strtol(BASALT_CFG_BME280_I2C_ADDRESS, NULL, 0);
    if (v <= 0 || v > 0x7F) v = 0x76;
    return (uint8_t)v;
}

static bool bsh_bme280_i2c_ensure(char *err, size_t err_len) {
    if (s_bme280_i2c_ready) return true;
    if (BASALT_PIN_I2C_SDA < 0 || BASALT_PIN_I2C_SCL < 0) {
        if (err && err_len) snprintf(err, err_len, "invalid I2C pins (sda=%d scl=%d)", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
        return false;
    }
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BASALT_PIN_I2C_SDA,
        .scl_io_num = BASALT_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_param_config failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        s_bme280_i2c_ready = true;
        return true;
    }
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_driver_install failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_bme280_i2c_ready = true;
    return true;
}

static bool bsh_bme280_read_regs(uint8_t reg, uint8_t *buf, size_t len, char *err, size_t err_len) {
    if (!buf || len == 0) {
        if (err && err_len) snprintf(err, err_len, "invalid read buffer");
        return false;
    }
    if (!bsh_bme280_i2c_ensure(err, err_len)) return false;
    const uint8_t addr = bsh_bme280_addr();
    esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, addr, &reg, 1, buf, len, pdMS_TO_TICKS(60));
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "register read 0x%02X failed (%s)", reg, esp_err_to_name(ret));
        return false;
    }
    return true;
}

static bool bsh_bme280_probe(uint8_t *chip_id, char *err, size_t err_len) {
    uint8_t who = 0;
    if (!bsh_bme280_read_regs(0xD0, &who, 1, err, err_len)) return false;
    if (chip_id) *chip_id = who;
    if (who != 0x60) {
        if (err && err_len) snprintf(err, err_len, "unexpected chip id 0x%02X", who);
        return false;
    }
    return true;
}

static void bsh_cmd_bme280_status(void) {
    basalt_printf("bme280.enabled: yes\n");
    basalt_printf("bme280.addr_cfg: %s\n", BASALT_CFG_BME280_I2C_ADDRESS);
    basalt_printf("bme280.i2c_pins: sda=%d scl=%d\n", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
    uint8_t chip = 0;
    char err[96];
    if (!bsh_bme280_probe(&chip, err, sizeof(err))) {
        basalt_printf("bme280.probe: fail (%s)\n", err);
        return;
    }
    basalt_printf("bme280.probe: ok\n");
    basalt_printf("bme280.chip_id: 0x%02X\n", chip);
}

static void bsh_cmd_bme280_read(void) {
    char err[96];
    uint8_t chip = 0;
    if (!bsh_bme280_probe(&chip, err, sizeof(err))) {
        basalt_printf("bme280 read: fail (%s)\n", err);
        return;
    }

    uint8_t data[8] = {0};
    if (!bsh_bme280_read_regs(0xF7, data, sizeof(data), err, sizeof(err))) {
        basalt_printf("bme280 read: fail (%s)\n", err);
        return;
    }

    uint32_t raw_press = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)(data[2] >> 4));
    uint32_t raw_temp = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)(data[5] >> 4));
    uint32_t raw_hum = ((uint32_t)data[6] << 8) | (uint32_t)data[7];

    basalt_printf("bme280 read: chip_id=0x%02X\n", chip);
    basalt_printf("bme280 raw: temp=%lu press=%lu hum=%lu\n", (unsigned long)raw_temp, (unsigned long)raw_press, (unsigned long)raw_hum);
    basalt_printf("bme280 note: values are raw ADC outputs; compensation/calibration conversion is follow-up scope.\n");
}

#else
static void bsh_cmd_bme280_status(void) {
    basalt_printf("bme280: unavailable (enable bme280+i2c drivers)\n");
}
static void bsh_cmd_bme280_read(void) {
    basalt_printf("bme280 read: unavailable (enable bme280+i2c drivers)\n");
}
#endif

#if BASALT_ENABLE_ADS1115 && BASALT_ENABLE_I2C
static bool s_ads1115_i2c_ready = false;

static uint8_t bsh_ads1115_addr(void) {
    long v = strtol(BASALT_CFG_ADS1115_ADDRESS, NULL, 0);
    if (v <= 0 || v > 0x7F) v = 0x48;
    return (uint8_t)v;
}

static uint8_t bsh_ads1115_pga_bits(void) {
    const long pga = (long)BASALT_CFG_ADS1115_PGA_MV;
    if (pga >= 6144) return 0;
    if (pga >= 4096) return 1;
    if (pga >= 2048) return 2;
    if (pga >= 1024) return 3;
    if (pga >= 512) return 4;
    return 5;
}

static int32_t bsh_ads1115_pga_mv(void) {
    const long pga = (long)BASALT_CFG_ADS1115_PGA_MV;
    if (pga >= 6144) return 6144;
    if (pga >= 4096) return 4096;
    if (pga >= 2048) return 2048;
    if (pga >= 1024) return 1024;
    if (pga >= 512) return 512;
    return 256;
}

static uint8_t bsh_ads1115_dr_bits(void) {
    const long sps = (long)BASALT_CFG_ADS1115_SAMPLE_RATE_SPS;
    if (sps <= 8) return 0;
    if (sps <= 16) return 1;
    if (sps <= 32) return 2;
    if (sps <= 64) return 3;
    if (sps <= 128) return 4;
    if (sps <= 250) return 5;
    if (sps <= 475) return 6;
    return 7;
}

static uint16_t bsh_ads1115_wait_ms(void) {
    const long sps = (long)BASALT_CFG_ADS1115_SAMPLE_RATE_SPS;
    if (sps >= 860) return 2;
    if (sps >= 475) return 3;
    if (sps >= 250) return 5;
    if (sps >= 128) return 9;
    if (sps >= 64) return 17;
    if (sps >= 32) return 33;
    if (sps >= 16) return 65;
    return 130;
}

static bool bsh_ads1115_is_continuous(void) {
    return strcmp(BASALT_CFG_ADS1115_MODE, "continuous") == 0;
}

static bool bsh_ads1115_i2c_ensure(char *err, size_t err_len) {
    if (s_ads1115_i2c_ready) return true;
    if (BASALT_PIN_I2C_SDA < 0 || BASALT_PIN_I2C_SCL < 0) {
        if (err && err_len) snprintf(err, err_len, "invalid I2C pins (sda=%d scl=%d)", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
        return false;
    }
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BASALT_PIN_I2C_SDA,
        .scl_io_num = BASALT_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_param_config failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        s_ads1115_i2c_ready = true;
        return true;
    }
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_driver_install failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_ads1115_i2c_ready = true;
    return true;
}

static bool bsh_ads1115_write_reg16(uint8_t reg, uint16_t value, char *err, size_t err_len) {
    uint8_t payload[3] = {
        reg,
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)(value & 0xFF),
    };
    esp_err_t ret = i2c_master_write_to_device(
        I2C_NUM_0, bsh_ads1115_addr(), payload, sizeof(payload), pdMS_TO_TICKS(60));
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "write reg 0x%02X failed (%s)", reg, esp_err_to_name(ret));
        return false;
    }
    return true;
}

static bool bsh_ads1115_read_reg16(uint8_t reg, uint16_t *out, char *err, size_t err_len) {
    if (!out) {
        if (err && err_len) snprintf(err, err_len, "invalid output pointer");
        return false;
    }
    uint8_t data[2] = {0};
    esp_err_t ret = i2c_master_write_read_device(
        I2C_NUM_0, bsh_ads1115_addr(), &reg, 1, data, sizeof(data), pdMS_TO_TICKS(60));
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "read reg 0x%02X failed (%s)", reg, esp_err_to_name(ret));
        return false;
    }
    *out = (uint16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    return true;
}

static bool bsh_ads1115_probe(uint16_t *cfg, char *err, size_t err_len) {
    if (!bsh_ads1115_i2c_ensure(err, err_len)) return false;
    uint16_t config = 0;
    if (!bsh_ads1115_read_reg16(0x01, &config, err, err_len)) return false;
    if (cfg) *cfg = config;
    return true;
}

static bool bsh_ads1115_read_channel(uint8_t channel, int16_t *raw_out, int32_t *mv_out, char *err, size_t err_len) {
    if (channel > 3) {
        if (err && err_len) snprintf(err, err_len, "channel must be 0..3");
        return false;
    }
    if (!raw_out || !mv_out) {
        if (err && err_len) snprintf(err, err_len, "invalid output pointers");
        return false;
    }

    uint16_t cfg = 0;
    cfg |= (1u << 15); // OS: start conversion
    cfg |= (uint16_t)((0x4u + (uint16_t)channel) & 0x7u) << 12; // AINx single-ended
    cfg |= (uint16_t)(bsh_ads1115_pga_bits() & 0x7u) << 9;
    cfg |= (uint16_t)(bsh_ads1115_is_continuous() ? 0u : 1u) << 8;
    cfg |= (uint16_t)(bsh_ads1115_dr_bits() & 0x7u) << 5;
    cfg |= 0x0003u; // disable comparator

    if (!bsh_ads1115_write_reg16(0x01, cfg, err, err_len)) return false;
    if (!bsh_ads1115_is_continuous()) vTaskDelay(pdMS_TO_TICKS(bsh_ads1115_wait_ms()));

    uint16_t conv = 0;
    if (!bsh_ads1115_read_reg16(0x00, &conv, err, err_len)) return false;

    int16_t raw = (int16_t)conv;
    int32_t mv = ((int32_t)raw * bsh_ads1115_pga_mv()) / 32768;
    *raw_out = raw;
    *mv_out = mv;
    return true;
}

static void bsh_cmd_ads1115_status(void) {
    basalt_printf("ads1115.enabled: yes\n");
    basalt_printf("ads1115.addr_cfg: %s\n", BASALT_CFG_ADS1115_ADDRESS);
    basalt_printf("ads1115.pga_mv_cfg: %ld\n", (long)bsh_ads1115_pga_mv());
    basalt_printf("ads1115.sample_rate_sps_cfg: %ld\n", (long)BASALT_CFG_ADS1115_SAMPLE_RATE_SPS);
    basalt_printf("ads1115.mode_cfg: %s\n", BASALT_CFG_ADS1115_MODE);
    basalt_printf("ads1115.i2c_pins: sda=%d scl=%d\n", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);

    uint16_t cfg = 0;
    char err[96];
    if (!bsh_ads1115_probe(&cfg, err, sizeof(err))) {
        basalt_printf("ads1115.probe: fail (%s)\n", err);
        return;
    }
    basalt_printf("ads1115.probe: ok\n");
    basalt_printf("ads1115.config_reg: 0x%04X\n", cfg);
}

static void bsh_cmd_ads1115_read(const char *channel_arg) {
    uint8_t channel = 0;
    if (channel_arg && channel_arg[0]) {
        char *end = NULL;
        long v = strtol(channel_arg, &end, 0);
        if (end == channel_arg || (end && *end != '\0') || v < 0 || v > 3) {
            basalt_printf("ads1115 read: invalid channel '%s' (expected 0..3)\n", channel_arg);
            return;
        }
        channel = (uint8_t)v;
    }

    char err[96];
    int16_t raw = 0;
    int32_t mv = 0;
    if (!bsh_ads1115_read_channel(channel, &raw, &mv, err, sizeof(err))) {
        basalt_printf("ads1115 read: fail (%s)\n", err);
        return;
    }

    basalt_printf("ads1115 read: ch=%u raw=%d mv=%ld\n", (unsigned)channel, (int)raw, (long)mv);
}
#else
static void bsh_cmd_ads1115_status(void) {
    basalt_printf("ads1115: unavailable (enable ads1115+i2c drivers)\n");
}
static void bsh_cmd_ads1115_read(const char *channel_arg) {
    (void)channel_arg;
    basalt_printf("ads1115 read: unavailable (enable ads1115+i2c drivers)\n");
}
#endif

#if BASALT_ENABLE_MCP23017 && BASALT_ENABLE_I2C
static bool s_mcp23017_i2c_ready = false;
static uint8_t s_mcp23017_iodir_a = 0xFF;
static uint8_t s_mcp23017_iodir_b = 0xFF;
static uint8_t s_mcp23017_gpio_a = 0x00;
static uint8_t s_mcp23017_gpio_b = 0x00;

static uint8_t bsh_mcp23017_addr(void) {
    long v = strtol(BASALT_CFG_MCP23017_ADDRESS, NULL, 0);
    if (v <= 0 || v > 0x7F) v = 0x20;
    return (uint8_t)v;
}

static bool bsh_mcp23017_default_output(void) {
    return strcmp(BASALT_CFG_MCP23017_DEFAULT_DIRECTION, "output") == 0;
}

static bool bsh_mcp23017_i2c_ensure(char *err, size_t err_len) {
    if (s_mcp23017_i2c_ready) return true;
    if (BASALT_PIN_I2C_SDA < 0 || BASALT_PIN_I2C_SCL < 0) {
        if (err && err_len) snprintf(err, err_len, "invalid I2C pins (sda=%d scl=%d)", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
        return false;
    }
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BASALT_PIN_I2C_SDA,
        .scl_io_num = BASALT_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_param_config failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        s_mcp23017_i2c_ready = true;
        return true;
    }
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "i2c_driver_install failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_mcp23017_i2c_ready = true;
    return true;
}

static bool bsh_mcp23017_write8(uint8_t reg, uint8_t val, char *err, size_t err_len) {
    uint8_t payload[2] = {reg, val};
    esp_err_t ret = i2c_master_write_to_device(
        I2C_NUM_0, bsh_mcp23017_addr(), payload, sizeof(payload), pdMS_TO_TICKS(60));
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "write reg 0x%02X failed (%s)", reg, esp_err_to_name(ret));
        return false;
    }
    return true;
}

static bool bsh_mcp23017_read8(uint8_t reg, uint8_t *out, char *err, size_t err_len) {
    if (!out) {
        if (err && err_len) snprintf(err, err_len, "invalid output pointer");
        return false;
    }
    esp_err_t ret = i2c_master_write_read_device(
        I2C_NUM_0, bsh_mcp23017_addr(), &reg, 1, out, 1, pdMS_TO_TICKS(60));
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "read reg 0x%02X failed (%s)", reg, esp_err_to_name(ret));
        return false;
    }
    return true;
}

static bool bsh_mcp23017_probe(char *err, size_t err_len) {
    if (!bsh_mcp23017_i2c_ensure(err, err_len)) return false;
    uint8_t iodira = 0;
    return bsh_mcp23017_read8(0x00, &iodira, err, err_len);
}

static bool bsh_mcp23017_probe_addr(uint8_t addr, uint8_t *iodira) {
    uint8_t reg = 0x00;
    uint8_t val = 0;
    esp_err_t ret = i2c_master_write_read_device(
        I2C_NUM_0, addr, &reg, 1, &val, 1, pdMS_TO_TICKS(40));
    if (ret != ESP_OK) return false;
    if (iodira) *iodira = val;
    return true;
}

static bool bsh_mcp23017_init_defaults(char *err, size_t err_len) {
    if (!bsh_mcp23017_probe(err, err_len)) return false;
    bool out_mode = bsh_mcp23017_default_output();
    s_mcp23017_iodir_a = out_mode ? 0x00 : 0xFF;
    s_mcp23017_iodir_b = out_mode ? 0x00 : 0xFF;
    if (!bsh_mcp23017_write8(0x00, s_mcp23017_iodir_a, err, err_len)) return false;
    if (!bsh_mcp23017_write8(0x01, s_mcp23017_iodir_b, err, err_len)) return false;
    if (!bsh_mcp23017_write8(0x12, s_mcp23017_gpio_a, err, err_len)) return false;
    if (!bsh_mcp23017_write8(0x13, s_mcp23017_gpio_b, err, err_len)) return false;
    return true;
}

static bool bsh_mcp23017_parse_pin(const char *arg, uint8_t *out) {
    if (!arg || !out) return false;
    char *end = NULL;
    long v = strtol(arg, &end, 0);
    if (end == arg || (end && *end != '\0') || v < 0 || v > 15) return false;
    *out = (uint8_t)v;
    return true;
}

static bool bsh_mcp23017_pin_mode(uint8_t pin, bool input, char *err, size_t err_len) {
    if (pin > 15) {
        if (err && err_len) snprintf(err, err_len, "pin must be 0..15");
        return false;
    }
    uint8_t bit = (uint8_t)(1u << (pin & 7u));
    if (pin < 8) {
        if (input) s_mcp23017_iodir_a |= bit; else s_mcp23017_iodir_a &= (uint8_t)~bit;
        return bsh_mcp23017_write8(0x00, s_mcp23017_iodir_a, err, err_len);
    }
    if (input) s_mcp23017_iodir_b |= bit; else s_mcp23017_iodir_b &= (uint8_t)~bit;
    return bsh_mcp23017_write8(0x01, s_mcp23017_iodir_b, err, err_len);
}

static bool bsh_mcp23017_pin_write(uint8_t pin, bool high, char *err, size_t err_len) {
    if (pin > 15) {
        if (err && err_len) snprintf(err, err_len, "pin must be 0..15");
        return false;
    }
    uint8_t bit = (uint8_t)(1u << (pin & 7u));
    if (pin < 8) {
        if (high) s_mcp23017_gpio_a |= bit; else s_mcp23017_gpio_a &= (uint8_t)~bit;
        return bsh_mcp23017_write8(0x12, s_mcp23017_gpio_a, err, err_len);
    }
    if (high) s_mcp23017_gpio_b |= bit; else s_mcp23017_gpio_b &= (uint8_t)~bit;
    return bsh_mcp23017_write8(0x13, s_mcp23017_gpio_b, err, err_len);
}

static int bsh_mcp23017_pin_read(uint8_t pin, char *err, size_t err_len) {
    if (pin > 15) {
        if (err && err_len) snprintf(err, err_len, "pin must be 0..15");
        return -1;
    }
    uint8_t val = 0;
    if (!bsh_mcp23017_read8((pin < 8) ? 0x12 : 0x13, &val, err, err_len)) return -1;
    return (val & (uint8_t)(1u << (pin & 7u))) ? 1 : 0;
}

static void bsh_cmd_mcp23017(const char *sub, const char *arg1, const char *arg2) {
    char err[96];
    if (!sub || strcmp(sub, "status") == 0) {
        basalt_printf("mcp23017.enabled: yes\n");
        basalt_printf("mcp23017.addr_cfg: %s\n", BASALT_CFG_MCP23017_ADDRESS);
        basalt_printf("mcp23017.default_direction_cfg: %s\n", BASALT_CFG_MCP23017_DEFAULT_DIRECTION);
        basalt_printf("mcp23017.i2c_pins: sda=%d scl=%d\n", BASALT_PIN_I2C_SDA, BASALT_PIN_I2C_SCL);
        if (!bsh_mcp23017_probe(err, sizeof(err))) {
            basalt_printf("mcp23017.probe: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017.probe: ok\n");
        uint8_t iodira = 0, iodirb = 0, gpioa = 0, gpiob = 0;
        if (bsh_mcp23017_read8(0x00, &iodira, err, sizeof(err)) &&
            bsh_mcp23017_read8(0x01, &iodirb, err, sizeof(err)) &&
            bsh_mcp23017_read8(0x12, &gpioa, err, sizeof(err)) &&
            bsh_mcp23017_read8(0x13, &gpiob, err, sizeof(err))) {
            basalt_printf("mcp23017.iodir: A=0x%02X B=0x%02X\n", iodira, iodirb);
            basalt_printf("mcp23017.gpio: A=0x%02X B=0x%02X\n", gpioa, gpiob);
            s_mcp23017_iodir_a = iodira;
            s_mcp23017_iodir_b = iodirb;
            s_mcp23017_gpio_a = gpioa;
            s_mcp23017_gpio_b = gpiob;
        }
        return;
    }

    if (!bsh_mcp23017_i2c_ensure(err, sizeof(err))) {
        basalt_printf("mcp23017: %s\n", err);
        return;
    }

    if (strcmp(sub, "probe") == 0) {
        if (!bsh_mcp23017_probe(err, sizeof(err))) {
            basalt_printf("mcp23017 probe: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 probe: ok\n");
        return;
    }

    if (strcmp(sub, "scan") == 0) {
        basalt_printf("mcp23017 scan: checking 0x20..0x27\n");
        bool any = false;
        for (uint8_t addr = 0x20; addr <= 0x27; ++addr) {
            uint8_t iodira = 0;
            if (bsh_mcp23017_probe_addr(addr, &iodira)) {
                any = true;
                basalt_printf("mcp23017 scan: found addr=0x%02X iodira=0x%02X\n", addr, iodira);
            }
        }
        if (!any) basalt_printf("mcp23017 scan: no device response in 0x20..0x27\n");
        return;
    }

    if (strcmp(sub, "read") == 0) {
        uint8_t reg = 0;
        if (!arg1 || !arg1[0]) {
            basalt_printf("mcp23017 read: missing reg\n");
            return;
        }
        char *end = NULL;
        long v = strtol(arg1, &end, 0);
        if (end == arg1 || (end && *end != '\0') || v < 0 || v > 0xFF) {
            basalt_printf("mcp23017 read: invalid reg '%s'\n", arg1);
            return;
        }
        reg = (uint8_t)v;
        uint8_t val = 0;
        if (!bsh_mcp23017_read8(reg, &val, err, sizeof(err))) {
            basalt_printf("mcp23017 read: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 read: reg=0x%02X val=0x%02X\n", reg, val);
        return;
    }

    if (strcmp(sub, "write") == 0) {
        if (!arg1 || !arg2) {
            basalt_printf("usage: mcp23017 write <reg> <val>\n");
            return;
        }
        char *end1 = NULL, *end2 = NULL;
        long reg_v = strtol(arg1, &end1, 0);
        long val_v = strtol(arg2, &end2, 0);
        if (end1 == arg1 || (end1 && *end1 != '\0') || reg_v < 0 || reg_v > 0xFF ||
            end2 == arg2 || (end2 && *end2 != '\0') || val_v < 0 || val_v > 0xFF) {
            basalt_printf("mcp23017 write: invalid args\n");
            return;
        }
        uint8_t reg = (uint8_t)reg_v;
        uint8_t val = (uint8_t)val_v;
        if (!bsh_mcp23017_write8(reg, val, err, sizeof(err))) {
            basalt_printf("mcp23017 write: fail (%s)\n", err);
            return;
        }
        uint8_t verify = 0;
        if (!bsh_mcp23017_read8(reg, &verify, err, sizeof(err))) {
            basalt_printf("mcp23017 write: verify read failed (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 write: reg=0x%02X val=0x%02X verify=0x%02X\n", reg, val, verify);
        return;
    }

    if (strcmp(sub, "pinmode") == 0) {
        uint8_t pin = 0;
        if (!bsh_mcp23017_parse_pin(arg1, &pin) || !arg2) {
            basalt_printf("usage: mcp23017 pinmode <0-15> <in|out>\n");
            return;
        }
        bool input = (strcmp(arg2, "in") == 0 || strcmp(arg2, "input") == 0);
        bool output = (strcmp(arg2, "out") == 0 || strcmp(arg2, "output") == 0);
        if (!input && !output) {
            basalt_printf("mcp23017 pinmode: mode must be in|out\n");
            return;
        }
        if (!bsh_mcp23017_pin_mode(pin, input, err, sizeof(err))) {
            basalt_printf("mcp23017 pinmode: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 pinmode: pin=%u mode=%s\n", (unsigned)pin, input ? "in" : "out");
        return;
    }

    if (strcmp(sub, "pinwrite") == 0) {
        uint8_t pin = 0;
        if (!bsh_mcp23017_parse_pin(arg1, &pin) || !arg2) {
            basalt_printf("usage: mcp23017 pinwrite <0-15> <0|1>\n");
            return;
        }
        bool high = (strcmp(arg2, "1") == 0 || strcmp(arg2, "high") == 0 || strcmp(arg2, "on") == 0);
        bool low = (strcmp(arg2, "0") == 0 || strcmp(arg2, "low") == 0 || strcmp(arg2, "off") == 0);
        if (!high && !low) {
            basalt_printf("mcp23017 pinwrite: value must be 0|1\n");
            return;
        }
        if (!bsh_mcp23017_pin_write(pin, high, err, sizeof(err))) {
            basalt_printf("mcp23017 pinwrite: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 pinwrite: pin=%u val=%d\n", (unsigned)pin, high ? 1 : 0);
        return;
    }

    if (strcmp(sub, "pinread") == 0) {
        uint8_t pin = 0;
        if (!bsh_mcp23017_parse_pin(arg1, &pin)) {
            basalt_printf("usage: mcp23017 pinread <0-15>\n");
            return;
        }
        int v = bsh_mcp23017_pin_read(pin, err, sizeof(err));
        if (v < 0) {
            basalt_printf("mcp23017 pinread: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 pinread: pin=%u val=%d\n", (unsigned)pin, v);
        return;
    }

    if (strcmp(sub, "test") == 0) {
        if (!bsh_mcp23017_init_defaults(err, sizeof(err))) {
            basalt_printf("mcp23017 test: init failed (%s)\n", err);
            return;
        }
        for (uint8_t pin = 0; pin < 4; ++pin) {
            if (!bsh_mcp23017_pin_mode(pin, false, err, sizeof(err)) ||
                !bsh_mcp23017_pin_write(pin, true, err, sizeof(err))) {
                basalt_printf("mcp23017 test: fail (%s)\n", err);
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(120));
            if (!bsh_mcp23017_pin_write(pin, false, err, sizeof(err))) {
                basalt_printf("mcp23017 test: fail (%s)\n", err);
                return;
            }
        }
        if (!bsh_mcp23017_pin_mode(7, true, err, sizeof(err))) {
            basalt_printf("mcp23017 test: fail (%s)\n", err);
            return;
        }
        int in7 = bsh_mcp23017_pin_read(7, err, sizeof(err));
        if (in7 < 0) {
            basalt_printf("mcp23017 test: fail (%s)\n", err);
            return;
        }
        basalt_printf("mcp23017 test: IN7=%d\n", in7);
        basalt_printf("mcp23017 test: done\n");
        return;
    }

    basalt_printf("usage: mcp23017 [status|probe|scan|test|read <reg>|write <reg> <val>|pinmode <0-15> <in|out>|pinwrite <0-15> <0|1>|pinread <0-15>]\n");
}
#else
static void bsh_cmd_mcp23017(const char *sub, const char *arg1, const char *arg2) {
    (void)sub;
    (void)arg1;
    (void)arg2;
    basalt_printf("mcp23017: unavailable (enable mcp23017+i2c drivers)\n");
}
#endif

#if BASALT_ENABLE_TP4056
static bool s_tp4056_gpio_ready = false;

static bool bsh_tp4056_status_active_low(void) {
    return BASALT_CFG_TP4056_STATUS_PINS_ACTIVE_LOW ? true : false;
}

static bool bsh_tp4056_ce_present(void) {
    return BASALT_CFG_TP4056_CE_PIN_PRESENT && (BASALT_PIN_TP4056_CE >= 0);
}

static bool bsh_tp4056_ce_active_high(void) {
    return BASALT_CFG_TP4056_CE_ACTIVE_HIGH ? true : false;
}

static bool bsh_tp4056_gpio_ensure(char *err, size_t err_len) {
    if (s_tp4056_gpio_ready) return true;
    if (BASALT_PIN_TP4056_CHRG < 0 && BASALT_PIN_TP4056_DONE < 0) {
        if (err && err_len) snprintf(err, err_len, "no status pins configured (chrg=%d done=%d)", BASALT_PIN_TP4056_CHRG, BASALT_PIN_TP4056_DONE);
        return false;
    }

    esp_err_t ret = ESP_OK;
    if (BASALT_PIN_TP4056_CHRG >= 0) {
        ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_TP4056_CHRG);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_TP4056_CHRG, GPIO_MODE_INPUT);
        if (ret == ESP_OK) ret = gpio_set_pull_mode((gpio_num_t)BASALT_PIN_TP4056_CHRG, GPIO_PULLUP_ONLY);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "chrg pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (BASALT_PIN_TP4056_DONE >= 0) {
        ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_TP4056_DONE);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_TP4056_DONE, GPIO_MODE_INPUT);
        if (ret == ESP_OK) ret = gpio_set_pull_mode((gpio_num_t)BASALT_PIN_TP4056_DONE, GPIO_PULLUP_ONLY);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "done pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (bsh_tp4056_ce_present()) {
        ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_TP4056_CE);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_TP4056_CE, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "ce pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    s_tp4056_gpio_ready = true;
    return true;
}

static bool bsh_tp4056_set_enable(bool enabled, char *err, size_t err_len) {
    if (!bsh_tp4056_ce_present()) {
        if (err && err_len) snprintf(err, err_len, "CE pin not configured");
        return false;
    }
    const bool pin_high = bsh_tp4056_ce_active_high() ? enabled : !enabled;
    esp_err_t ret = gpio_set_level((gpio_num_t)BASALT_PIN_TP4056_CE, pin_high ? 1 : 0);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "set CE failed (%s)", esp_err_to_name(ret));
        return false;
    }
    return true;
}

static int bsh_tp4056_read_logic_pin(int pin) {
    if (pin < 0) return -1;
    int raw = gpio_get_level((gpio_num_t)pin);
    return bsh_tp4056_status_active_low() ? (raw == 0 ? 1 : 0) : (raw != 0 ? 1 : 0);
}
#endif

static void bsh_cmd_tp4056(const char *sub) {
#if BASALT_ENABLE_TP4056
    char err[96];
    if (!bsh_tp4056_gpio_ensure(err, sizeof(err))) {
        basalt_printf("tp4056: %s\n", err);
        return;
    }
    if (sub && sub[0]) {
        if (strcmp(sub, "on") == 0) {
            if (!bsh_tp4056_set_enable(true, err, sizeof(err))) {
                basalt_printf("tp4056 on: %s\n", err);
                return;
            }
            basalt_printf("tp4056.ce: enabled\n");
            vTaskDelay(pdMS_TO_TICKS(2));
        } else if (strcmp(sub, "off") == 0) {
            if (!bsh_tp4056_set_enable(false, err, sizeof(err))) {
                basalt_printf("tp4056 off: %s\n", err);
                return;
            }
            basalt_printf("tp4056.ce: disabled\n");
            vTaskDelay(pdMS_TO_TICKS(2));
        } else if (strcmp(sub, "status") != 0) {
            basalt_printf("usage: tp4056 [status|on|off]\n");
            return;
        }
    }

    const int charging = bsh_tp4056_read_logic_pin(BASALT_PIN_TP4056_CHRG);
    const int done = bsh_tp4056_read_logic_pin(BASALT_PIN_TP4056_DONE);

    basalt_printf("tp4056.enabled: yes\n");
    basalt_printf("tp4056.pins: chrg=%d done=%d ce=%d\n", BASALT_PIN_TP4056_CHRG, BASALT_PIN_TP4056_DONE, BASALT_PIN_TP4056_CE);
    basalt_printf("tp4056.status_active_low: %s\n", bsh_tp4056_status_active_low() ? "yes" : "no");
    basalt_printf("tp4056.charging: %s\n", charging < 0 ? "n/a" : (charging ? "yes" : "no"));
    basalt_printf("tp4056.done: %s\n", done < 0 ? "n/a" : (done ? "yes" : "no"));
    if (charging >= 0 && done >= 0) {
        const char *state = "standby/unknown";
        if (charging && !done) state = "charging";
        else if (!charging && done) state = "charge_complete";
        else if (!charging && !done) state = "fault_or_no_battery";
        basalt_printf("tp4056.state: %s\n", state);
    }
    if (bsh_tp4056_ce_present()) {
        int ce_level = gpio_get_level((gpio_num_t)BASALT_PIN_TP4056_CE);
        bool ce_enabled = bsh_tp4056_ce_active_high() ? (ce_level != 0) : (ce_level == 0);
        basalt_printf("tp4056.ce_pin_present: yes\n");
        basalt_printf("tp4056.ce_active_high: %s\n", bsh_tp4056_ce_active_high() ? "yes" : "no");
        basalt_printf("tp4056.ce_enabled: %s\n", ce_enabled ? "yes" : "no");
    } else {
        basalt_printf("tp4056.ce_pin_present: no\n");
    }
#else
    (void)sub;
    basalt_printf("tp4056: unavailable (enable tp4056+gpio drivers)\n");
#endif
}

#if BASALT_ENABLE_MCP2544FD
static bool s_mcp2544fd_gpio_ready = false;

static bool bsh_mcp2544fd_stby_active_high(void) {
    return BASALT_CFG_MCP2544FD_STBY_ACTIVE_HIGH ? true : false;
}

static bool bsh_mcp2544fd_en_active_high(void) {
    return BASALT_CFG_MCP2544FD_EN_ACTIVE_HIGH ? true : false;
}

static bool bsh_mcp2544fd_gpio_ensure(char *err, size_t err_len) {
    if (s_mcp2544fd_gpio_ready) return true;
    if (BASALT_PIN_CAN_STBY < 0 && BASALT_PIN_CAN_EN < 0) {
        if (err && err_len) snprintf(err, err_len, "no control pins configured (stby=%d en=%d)", BASALT_PIN_CAN_STBY, BASALT_PIN_CAN_EN);
        return false;
    }

    esp_err_t ret = ESP_OK;
    if (BASALT_PIN_CAN_STBY >= 0) {
        ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_CAN_STBY);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_CAN_STBY, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "stby pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (BASALT_PIN_CAN_EN >= 0) {
        ret = gpio_reset_pin((gpio_num_t)BASALT_PIN_CAN_EN);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)BASALT_PIN_CAN_EN, GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "en pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    s_mcp2544fd_gpio_ready = true;
    return true;
}

static bool bsh_mcp2544fd_apply_state(bool enabled, bool standby, char *err, size_t err_len) {
    if (BASALT_PIN_CAN_EN >= 0) {
        const bool en_level = bsh_mcp2544fd_en_active_high() ? enabled : !enabled;
        esp_err_t ret = gpio_set_level((gpio_num_t)BASALT_PIN_CAN_EN, en_level ? 1 : 0);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "set EN failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (BASALT_PIN_CAN_STBY >= 0) {
        const bool stby_level = bsh_mcp2544fd_stby_active_high() ? standby : !standby;
        esp_err_t ret = gpio_set_level((gpio_num_t)BASALT_PIN_CAN_STBY, stby_level ? 1 : 0);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "set STBY failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    return true;
}

static void bsh_cmd_mcp2544fd(const char *sub) {
    char err[96];
    if (!bsh_mcp2544fd_gpio_ensure(err, sizeof(err))) {
        basalt_printf("mcp2544fd: %s\n", err);
        return;
    }

    if (sub && sub[0]) {
        if (strcmp(sub, "on") == 0) {
            if (!bsh_mcp2544fd_apply_state(true, false, err, sizeof(err))) {
                basalt_printf("mcp2544fd on: %s\n", err);
                return;
            }
            basalt_printf("mcp2544fd: normal mode\n");
            vTaskDelay(pdMS_TO_TICKS(2));
        } else if (strcmp(sub, "off") == 0) {
            if (!bsh_mcp2544fd_apply_state(false, true, err, sizeof(err))) {
                basalt_printf("mcp2544fd off: %s\n", err);
                return;
            }
            basalt_printf("mcp2544fd: transceiver disabled\n");
            vTaskDelay(pdMS_TO_TICKS(2));
        } else if (strcmp(sub, "standby") == 0) {
            if (!bsh_mcp2544fd_apply_state(true, true, err, sizeof(err))) {
                basalt_printf("mcp2544fd standby: %s\n", err);
                return;
            }
            basalt_printf("mcp2544fd: standby mode\n");
            vTaskDelay(pdMS_TO_TICKS(2));
        } else if (strcmp(sub, "status") != 0) {
            basalt_printf("usage: mcp2544fd [status|on|off|standby]\n");
            return;
        }
    }

    basalt_printf("mcp2544fd.enabled: yes\n");
    basalt_printf("mcp2544fd.pins: stby=%d en=%d\n", BASALT_PIN_CAN_STBY, BASALT_PIN_CAN_EN);
    basalt_printf("mcp2544fd.stby_active_high: %s\n", bsh_mcp2544fd_stby_active_high() ? "yes" : "no");
    basalt_printf("mcp2544fd.en_active_high: %s\n", bsh_mcp2544fd_en_active_high() ? "yes" : "no");
    if (BASALT_PIN_CAN_STBY >= 0) {
        const int stby_raw = gpio_get_level((gpio_num_t)BASALT_PIN_CAN_STBY);
        const bool standby = bsh_mcp2544fd_stby_active_high() ? (stby_raw != 0) : (stby_raw == 0);
        basalt_printf("mcp2544fd.standby: %s\n", standby ? "yes" : "no");
    } else {
        basalt_printf("mcp2544fd.standby: n/a (no STBY pin)\n");
    }
    if (BASALT_PIN_CAN_EN >= 0) {
        const int en_raw = gpio_get_level((gpio_num_t)BASALT_PIN_CAN_EN);
        const bool enabled = bsh_mcp2544fd_en_active_high() ? (en_raw != 0) : (en_raw == 0);
        basalt_printf("mcp2544fd.phy_enabled: %s\n", enabled ? "yes" : "no");
    } else {
        basalt_printf("mcp2544fd.phy_enabled: n/a (no EN pin)\n");
    }
}
#else
static void bsh_cmd_mcp2544fd(const char *sub) {
    (void)sub;
    basalt_printf("mcp2544fd: unavailable (enable mcp2544fd+gpio drivers)\n");
}
#endif

#if BASALT_ENABLE_ULN2003
static bool s_uln2003_gpio_ready = false;

static bool bsh_uln2003_active_high(void) {
    return BASALT_CFG_ULN2003_ACTIVE_HIGH ? true : false;
}

static bool bsh_uln2003_gpio_ensure(char *err, size_t err_len) {
    if (s_uln2003_gpio_ready) return true;
    const int pins[4] = {BASALT_PIN_ULN2003_IN1, BASALT_PIN_ULN2003_IN2, BASALT_PIN_ULN2003_IN3, BASALT_PIN_ULN2003_IN4};
    for (int i = 0; i < 4; ++i) {
        if (pins[i] < 0) {
            if (err && err_len) snprintf(err, err_len, "missing ULN2003 pin IN%d", i + 1);
            return false;
        }
    }
    for (int i = 0; i < 4; ++i) {
        esp_err_t ret = gpio_reset_pin((gpio_num_t)pins[i]);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)pins[i], GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "pin IN%d init failed (%s)", i + 1, esp_err_to_name(ret));
            return false;
        }
    }
    s_uln2003_gpio_ready = true;
    return true;
}

static void bsh_uln2003_apply_mask(uint8_t mask) {
    const int pins[4] = {BASALT_PIN_ULN2003_IN1, BASALT_PIN_ULN2003_IN2, BASALT_PIN_ULN2003_IN3, BASALT_PIN_ULN2003_IN4};
    const bool ah = bsh_uln2003_active_high();
    for (int i = 0; i < 4; ++i) {
        const bool on = (mask & (1u << i)) != 0;
        int level = ah ? (on ? 1 : 0) : (on ? 0 : 1);
        gpio_set_level((gpio_num_t)pins[i], level);
    }
}

static void bsh_cmd_uln2003(const char *sub, const char *arg1, const char *arg2) {
    char err[96];
    if (!bsh_uln2003_gpio_ensure(err, sizeof(err))) {
        basalt_printf("uln2003: %s\n", err);
        return;
    }

    if (!sub || strcmp(sub, "status") == 0) {
        basalt_printf("uln2003.enabled: yes\n");
        basalt_printf("uln2003.pins: in1=%d in2=%d in3=%d in4=%d\n",
            BASALT_PIN_ULN2003_IN1, BASALT_PIN_ULN2003_IN2, BASALT_PIN_ULN2003_IN3, BASALT_PIN_ULN2003_IN4);
        basalt_printf("uln2003.active_high: %s\n", bsh_uln2003_active_high() ? "yes" : "no");
        basalt_printf("uln2003.default_step_delay_ms: %d\n", (int)BASALT_CFG_ULN2003_STEP_DELAY_MS);
        return;
    }

    if (strcmp(sub, "off") == 0) {
        bsh_uln2003_apply_mask(0);
        basalt_printf("uln2003: coils off\n");
        return;
    }

    if (strcmp(sub, "test") == 0) {
        static const uint8_t seq[4] = {0x01, 0x02, 0x04, 0x08};
        static const char *names[4] = {"IN1", "IN2", "IN3", "IN4"};
        basalt_printf("uln2003 test: energizing one coil at a time (300ms each)\n");
        for (int i = 0; i < 4; ++i) {
            basalt_printf("uln2003 test: %s on\n", names[i]);
            bsh_uln2003_apply_mask(seq[i]);
            vTaskDelay(pdMS_TO_TICKS(300));
            bsh_uln2003_apply_mask(0);
            vTaskDelay(pdMS_TO_TICKS(120));
        }
        basalt_printf("uln2003 test: done\n");
        return;
    }

    if (strcmp(sub, "step") == 0) {
        if (!arg1 || !arg1[0]) {
            basalt_printf("usage: uln2003 step <steps> [delay_ms]\n");
            return;
        }
        int steps = (int)strtol(arg1, NULL, 10);
        if (steps == 0) {
            basalt_printf("uln2003 step: no movement (steps=0)\n");
            return;
        }
        int delay_ms = BASALT_CFG_ULN2003_STEP_DELAY_MS;
        if (arg2 && arg2[0]) delay_ms = (int)strtol(arg2, NULL, 10);
        if (delay_ms < 1) delay_ms = 1;
        if (delay_ms > 100) delay_ms = 100;
        if (steps > 20000) steps = 20000;
        if (steps < -20000) steps = -20000;

        static const uint8_t seq[4] = {0x01, 0x02, 0x04, 0x08};
        int idx = 0;
        int dir = (steps > 0) ? 1 : -1;
        int count = steps > 0 ? steps : -steps;
        for (int i = 0; i < count; ++i) {
            bsh_uln2003_apply_mask(seq[idx]);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
            idx += dir;
            if (idx > 3) idx = 0;
            if (idx < 0) idx = 3;
        }
        bsh_uln2003_apply_mask(0);
        basalt_printf("uln2003 step: done (%d steps @ %dms)\n", steps, delay_ms);
        return;
    }

    basalt_printf("usage: uln2003 [status|off|test|step <steps> [delay_ms]]\n");
}
#else
static void bsh_cmd_uln2003(const char *sub, const char *arg1, const char *arg2) {
    (void)sub;
    (void)arg1;
    (void)arg2;
    basalt_printf("uln2003: unavailable (enable uln2003+gpio drivers)\n");
}
#endif

#if BASALT_ENABLE_L298N
static bool s_l298n_gpio_ready = false;
static bool s_l298n_pwm_ready = false;
static bool s_l298n_pwm_supported = false;
static int s_l298n_speed_pct[2] = {BASALT_CFG_L298N_DEFAULT_SPEED_PCT, BASALT_CFG_L298N_DEFAULT_SPEED_PCT};

static bool bsh_l298n_in_active_high(void) {
    return BASALT_CFG_L298N_IN_ACTIVE_HIGH ? true : false;
}

static bool bsh_l298n_en_active_high(void) {
    return BASALT_CFG_L298N_EN_ACTIVE_HIGH ? true : false;
}

static int bsh_l298n_clamp_speed(int pct) {
    if (pct < 0) return 0;
    if (pct > 100) return 100;
    return pct;
}

static uint32_t bsh_l298n_pwm_max_duty(void) {
    return 255u;
}

static bool bsh_l298n_pwm_ensure(char *err, size_t err_len) {
    if (s_l298n_pwm_ready) return true;
    s_l298n_pwm_ready = true;
    if (BASALT_PIN_L298N_ENA < 0 && BASALT_PIN_L298N_ENB < 0) {
        s_l298n_pwm_supported = false;
        return true;
    }

    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = BASALT_CFG_L298N_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&tcfg);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "pwm timer init failed (%s)", esp_err_to_name(ret));
        return false;
    }

    if (BASALT_PIN_L298N_ENA >= 0) {
        ledc_channel_config_t ccfg = {
            .gpio_num = BASALT_PIN_L298N_ENA,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0,
        };
        ret = ledc_channel_config(&ccfg);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "pwm ENA init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }

    if (BASALT_PIN_L298N_ENB >= 0) {
        ledc_channel_config_t ccfg = {
            .gpio_num = BASALT_PIN_L298N_ENB,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_1,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0,
        };
        ret = ledc_channel_config(&ccfg);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "pwm ENB init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }

    s_l298n_pwm_supported = true;
    return true;
}

static bool bsh_l298n_gpio_ensure(char *err, size_t err_len) {
    if (s_l298n_gpio_ready) return true;
    const int required[4] = {BASALT_PIN_L298N_IN1, BASALT_PIN_L298N_IN2, BASALT_PIN_L298N_IN3, BASALT_PIN_L298N_IN4};
    for (int i = 0; i < 4; ++i) {
        if (required[i] < 0) {
            if (err && err_len) snprintf(err, err_len, "missing L298N IN%d pin", i + 1);
            return false;
        }
    }
    const int pins[6] = {
        BASALT_PIN_L298N_IN1, BASALT_PIN_L298N_IN2, BASALT_PIN_L298N_ENA,
        BASALT_PIN_L298N_IN3, BASALT_PIN_L298N_IN4, BASALT_PIN_L298N_ENB
    };
    for (int i = 0; i < 6; ++i) {
        if (pins[i] < 0) continue;
        esp_err_t ret = gpio_reset_pin((gpio_num_t)pins[i]);
        if (ret == ESP_OK) ret = gpio_set_direction((gpio_num_t)pins[i], GPIO_MODE_OUTPUT);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "pin init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (!bsh_l298n_pwm_ensure(err, err_len)) return false;
    s_l298n_gpio_ready = true;
    return true;
}

static void bsh_l298n_set_pin_logic(int pin, bool asserted, bool active_high) {
    if (pin < 0) return;
    int level = active_high ? (asserted ? 1 : 0) : (asserted ? 0 : 1);
    gpio_set_level((gpio_num_t)pin, level);
}

static void bsh_l298n_apply_enable(int ch, bool enabled) {
    int en = (ch == 0) ? BASALT_PIN_L298N_ENA : BASALT_PIN_L298N_ENB;
    if (en < 0) return;

    int pct = bsh_l298n_clamp_speed(s_l298n_speed_pct[ch]);
    uint32_t max = bsh_l298n_pwm_max_duty();
    uint32_t on_duty = enabled ? (uint32_t)((max * (uint32_t)pct) / 100u) : 0u;
    uint32_t duty = bsh_l298n_en_active_high() ? on_duty : (max - on_duty);

    if (!s_l298n_pwm_supported) {
        bool en_on = enabled && (pct > 0);
        bsh_l298n_set_pin_logic(en, en_on, bsh_l298n_en_active_high());
        return;
    }

    ledc_channel_t channel = (ch == 0) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

static void bsh_l298n_set_channel(int ch, int mode) {
    int in_a = (ch == 0) ? BASALT_PIN_L298N_IN1 : BASALT_PIN_L298N_IN3;
    int in_b = (ch == 0) ? BASALT_PIN_L298N_IN2 : BASALT_PIN_L298N_IN4;
    // mode: 0=stop, 1=fwd, 2=rev
    bool a_on = (mode == 1);
    bool b_on = (mode == 2);
    bsh_l298n_set_pin_logic(in_a, a_on, bsh_l298n_in_active_high());
    bsh_l298n_set_pin_logic(in_b, b_on, bsh_l298n_in_active_high());
    bsh_l298n_apply_enable(ch, mode != 0);
}

static int bsh_l298n_mode_from_str(const char *s) {
    if (!s) return -1;
    if (strcmp(s, "stop") == 0 || strcmp(s, "off") == 0) return 0;
    if (strcmp(s, "fwd") == 0 || strcmp(s, "forward") == 0) return 1;
    if (strcmp(s, "rev") == 0 || strcmp(s, "reverse") == 0) return 2;
    return -1;
}

static void bsh_cmd_l298n(const char *sub, const char *arg1, const char *arg2) {
    char err[96];
    if (!bsh_l298n_gpio_ensure(err, sizeof(err))) {
        basalt_printf("l298n: %s\n", err);
        return;
    }

    if (!sub || strcmp(sub, "status") == 0) {
        basalt_printf("l298n.enabled: yes\n");
        basalt_printf("l298n.pins.a: in1=%d in2=%d ena=%d\n", BASALT_PIN_L298N_IN1, BASALT_PIN_L298N_IN2, BASALT_PIN_L298N_ENA);
        basalt_printf("l298n.pins.b: in3=%d in4=%d enb=%d\n", BASALT_PIN_L298N_IN3, BASALT_PIN_L298N_IN4, BASALT_PIN_L298N_ENB);
        basalt_printf("l298n.in_active_high: %s\n", bsh_l298n_in_active_high() ? "yes" : "no");
        basalt_printf("l298n.en_active_high: %s\n", bsh_l298n_en_active_high() ? "yes" : "no");
        basalt_printf("l298n.speed.a_pct: %d\n", s_l298n_speed_pct[0]);
        basalt_printf("l298n.speed.b_pct: %d\n", s_l298n_speed_pct[1]);
        basalt_printf("l298n.pwm: %s\n", s_l298n_pwm_supported ? "enabled" : "disabled (no EN pins)");
        if (s_l298n_pwm_supported) basalt_printf("l298n.pwm.freq_hz: %d\n", BASALT_CFG_L298N_PWM_FREQ_HZ);
        return;
    }

    if (strcmp(sub, "stop") == 0) {
        bsh_l298n_set_channel(0, 0);
        bsh_l298n_set_channel(1, 0);
        basalt_printf("l298n: both channels stopped\n");
        return;
    }

    if (strcmp(sub, "speed") == 0) {
        if (!arg1 || !arg1[0]) {
            basalt_printf("usage: l298n speed <0-100>\n");
            return;
        }
        int pct = bsh_l298n_clamp_speed((int)strtol(arg1, NULL, 10));
        s_l298n_speed_pct[0] = pct;
        s_l298n_speed_pct[1] = pct;
        basalt_printf("l298n: speed a=%d%% b=%d%%\n", s_l298n_speed_pct[0], s_l298n_speed_pct[1]);
        return;
    }

    if (strcmp(sub, "test") == 0) {
        basalt_printf("l298n test: A fwd -> stop -> A rev -> stop -> B fwd -> stop -> B rev -> stop\n");
        bsh_l298n_set_channel(0, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        bsh_l298n_set_channel(0, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
        bsh_l298n_set_channel(0, 2);
        vTaskDelay(pdMS_TO_TICKS(300));
        bsh_l298n_set_channel(0, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
        bsh_l298n_set_channel(1, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        bsh_l298n_set_channel(1, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
        bsh_l298n_set_channel(1, 2);
        vTaskDelay(pdMS_TO_TICKS(300));
        bsh_l298n_set_channel(1, 0);
        basalt_printf("l298n test: done\n");
        return;
    }

    if (strcmp(sub, "a") == 0 || strcmp(sub, "b") == 0) {
        int ch = (strcmp(sub, "a") == 0) ? 0 : 1;
        if (arg1 && strcmp(arg1, "speed") == 0) {
            if (!arg2 || !arg2[0]) {
                basalt_printf("usage: l298n %s speed <0-100>\n", sub);
                return;
            }
            int pct = bsh_l298n_clamp_speed((int)strtol(arg2, NULL, 10));
            s_l298n_speed_pct[ch] = pct;
            basalt_printf("l298n: channel %s speed -> %d%%\n", sub, pct);
            return;
        }
        if (arg1 && arg1[0]) {
            int mode = bsh_l298n_mode_from_str(arg1);
            if (mode < 0) {
                basalt_printf("l298n: invalid mode '%s' (use fwd|rev|stop|speed)\n", arg1);
                return;
            }
            bsh_l298n_set_channel(ch, mode);
            basalt_printf("l298n: channel %s -> %s\n", sub, mode == 0 ? "stop" : (mode == 1 ? "fwd" : "rev"));
            return;
        }
    }

    basalt_printf("usage: l298n [status|stop|test|speed <0-100>|a <fwd|rev|stop|speed <0-100>>|b <fwd|rev|stop|speed <0-100>>]\n");
}
#else
static void bsh_cmd_l298n(const char *sub, const char *arg1, const char *arg2) {
    (void)sub;
    (void)arg1;
    (void)arg2;
    basalt_printf("l298n: unavailable (enable l298n+gpio drivers)\n");
}
#endif
#endif

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

#if BASALT_SHELL_LEVEL >= 3
#if BASALT_ENABLE_WIFI && defined(CONFIG_ESP_WIFI_ENABLED) && CONFIG_ESP_WIFI_ENABLED

#define BASALT_WIFI_CONNECTED_BIT BIT0
#define BASALT_WIFI_FAIL_BIT BIT1

static EventGroupHandle_t s_wifi_events = NULL;
static bool s_wifi_ready = false;
static bool s_wifi_connecting = false;
static bool s_wifi_connected = false;
static int s_wifi_retry_count = 0;
static const int s_wifi_retry_max = 5;
static char s_wifi_ssid[33] = {0};
static esp_netif_t *s_wifi_netif = NULL;
static esp_netif_ip_info_t s_wifi_last_ip = {0};

static const char *wifi_auth_mode_str(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "wep";
        case WIFI_AUTH_WPA_PSK: return "wpa";
        case WIFI_AUTH_WPA2_PSK: return "wpa2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "wpa/wpa2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2-ent";
        case WIFI_AUTH_WPA3_PSK: return "wpa3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa2/wpa3";
        case WIFI_AUTH_WAPI_PSK: return "wapi";
        default: return "unknown";
    }
}

static void basalt_wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        memset(&s_wifi_last_ip, 0, sizeof(s_wifi_last_ip));
        if (s_wifi_connecting && s_wifi_retry_count < s_wifi_retry_max) {
            s_wifi_retry_count++;
            esp_wifi_connect();
        } else if (s_wifi_connecting && s_wifi_events) {
            xEventGroupSetBits(s_wifi_events, BASALT_WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        if (evt) {
            s_wifi_last_ip = evt->ip_info;
        }
        s_wifi_connected = true;
        if (s_wifi_events) {
            xEventGroupSetBits(s_wifi_events, BASALT_WIFI_CONNECTED_BIT);
        }
    }
}

static bool basalt_wifi_init(char *err, size_t err_len) {
    if (s_wifi_ready) return true;

    if (!s_wifi_events) {
        s_wifi_events = xEventGroupCreate();
        if (!s_wifi_events) {
            if (err && err_len) snprintf(err, err_len, "event group allocation failed");
            return false;
        }
    }

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) snprintf(err, err_len, "esp_netif_init failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) snprintf(err, err_len, "event loop init failed (%s)", esp_err_to_name(ret));
        return false;
    }

    if (!s_wifi_netif) {
        s_wifi_netif = esp_netif_create_default_wifi_sta();
        if (!s_wifi_netif) {
            if (err && err_len) snprintf(err, err_len, "failed to create Wi-Fi netif");
            return false;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        if (err && err_len) snprintf(err, err_len, "esp_wifi_init failed (%s)", esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &basalt_wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "wifi event register failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &basalt_wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "ip event register failed (%s)", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "wifi storage set failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "wifi mode set failed (%s)", esp_err_to_name(ret));
        return false;
    }
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "wifi start failed (%s)", esp_err_to_name(ret));
        return false;
    }

    s_wifi_ready = true;
    return true;
}

static void bsh_cmd_wifi_status(void) {
    basalt_printf("wifi.enabled: yes\n");
    basalt_printf("wifi.initialized: %s\n", s_wifi_ready ? "yes" : "no");
    basalt_printf("wifi.connected: %s\n", s_wifi_connected ? "yes" : "no");
    basalt_printf("wifi.ssid: %s\n", s_wifi_ssid[0] ? s_wifi_ssid : "(none)");
    if (s_wifi_connected) {
        esp_netif_ip_info_t ip = {0};
        if (s_wifi_netif && esp_netif_get_ip_info(s_wifi_netif, &ip) == ESP_OK) {
            s_wifi_last_ip = ip;
        }
        basalt_printf("wifi.ipv4: " IPSTR "\n", IP2STR(&s_wifi_last_ip.ip));

        wifi_ap_record_t ap = {0};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            basalt_printf("wifi.rssi: %d\n", ap.rssi);
        }
    } else {
        basalt_printf("wifi.ipv4: (none)\n");
    }
}

static void bsh_cmd_wifi_connect(const char *ssid, const char *pass) {
    if (!ssid || !ssid[0]) {
        basalt_printf("wifi connect: missing ssid\n");
        basalt_printf("usage: wifi connect <ssid> [passphrase]\n");
        return;
    }
    if (strlen(ssid) > 32) {
        basalt_printf("wifi connect: ssid too long (max 32)\n");
        return;
    }
    if (pass && strlen(pass) > 63) {
        basalt_printf("wifi connect: passphrase too long (max 63)\n");
        return;
    }

    char err[128];
    if (!basalt_wifi_init(err, sizeof(err))) {
        basalt_printf("wifi connect: %s\n", err);
        return;
    }

    wifi_config_t cfg = {0};
    snprintf((char *)cfg.sta.ssid, sizeof(cfg.sta.ssid), "%s", ssid);
    snprintf((char *)cfg.sta.password, sizeof(cfg.sta.password), "%s", pass ? pass : "");
    cfg.sta.threshold.authmode = (pass && pass[0]) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (ret != ESP_OK) {
        basalt_printf("wifi connect: config failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    xEventGroupClearBits(s_wifi_events, BASALT_WIFI_CONNECTED_BIT | BASALT_WIFI_FAIL_BIT);
    s_wifi_connecting = true;
    s_wifi_connected = false;
    s_wifi_retry_count = 0;
    snprintf(s_wifi_ssid, sizeof(s_wifi_ssid), "%s", ssid);
    memset(&s_wifi_last_ip, 0, sizeof(s_wifi_last_ip));

    esp_wifi_disconnect();
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        s_wifi_connecting = false;
        basalt_printf("wifi connect: connect failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events,
        BASALT_WIFI_CONNECTED_BIT | BASALT_WIFI_FAIL_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );
    s_wifi_connecting = false;

    if (bits & BASALT_WIFI_CONNECTED_BIT) {
        basalt_printf("wifi connect: connected to %s\n", s_wifi_ssid);
        bsh_cmd_wifi_status();
    } else if (bits & BASALT_WIFI_FAIL_BIT) {
        basalt_printf("wifi connect: failed to connect to %s\n", s_wifi_ssid);
    } else {
        basalt_printf("wifi connect: timeout waiting for connection\n");
    }
}

static void bsh_cmd_wifi_scan(void) {
    char err[128];
    if (!basalt_wifi_init(err, sizeof(err))) {
        basalt_printf("wifi scan: %s\n", err);
        return;
    }

    wifi_scan_config_t scan_cfg = {0};
    esp_err_t ret = esp_wifi_scan_start(&scan_cfg, true);
    if (ret != ESP_OK) {
        basalt_printf("wifi scan: failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    uint16_t count = 20;
    wifi_ap_record_t records[20];
    memset(records, 0, sizeof(records));
    ret = esp_wifi_scan_get_ap_records(&count, records);
    if (ret != ESP_OK) {
        basalt_printf("wifi scan: read results failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    uint16_t total = 0;
    ret = esp_wifi_scan_get_ap_num(&total);
    if (ret == ESP_OK) {
        basalt_printf("wifi scan: %u network(s), showing %u\n", (unsigned)total, (unsigned)count);
    } else {
        basalt_printf("wifi scan: showing %u network(s)\n", (unsigned)count);
    }

    for (uint16_t i = 0; i < count; ++i) {
        const char *ssid = (const char *)records[i].ssid;
        if (!ssid || !ssid[0]) ssid = "<hidden>";
        basalt_printf("%2u) ssid=%s rssi=%d ch=%u auth=%s\n",
                      (unsigned)(i + 1),
                      ssid,
                      records[i].rssi,
                      (unsigned)records[i].primary,
                      wifi_auth_mode_str(records[i].authmode));
    }
}

static void bsh_cmd_wifi_reconnect(void) {
    char err[128];
    if (!basalt_wifi_init(err, sizeof(err))) {
        basalt_printf("wifi reconnect: %s\n", err);
        return;
    }

    wifi_config_t cfg = {0};
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &cfg);
    if (ret != ESP_OK) {
        basalt_printf("wifi reconnect: get config failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    if (cfg.sta.ssid[0] == '\0') {
        basalt_printf("wifi reconnect: no saved ssid; use 'wifi connect <ssid> [passphrase]'\n");
        return;
    }

    const char *ssid = (const char *)cfg.sta.ssid;
    snprintf(s_wifi_ssid, sizeof(s_wifi_ssid), "%s", ssid);
    memset(&s_wifi_last_ip, 0, sizeof(s_wifi_last_ip));
    s_wifi_retry_count = 0;
    s_wifi_connecting = true;
    s_wifi_connected = false;
    xEventGroupClearBits(s_wifi_events, BASALT_WIFI_CONNECTED_BIT | BASALT_WIFI_FAIL_BIT);

    esp_wifi_disconnect();
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        s_wifi_connecting = false;
        basalt_printf("wifi reconnect: connect failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events,
        BASALT_WIFI_CONNECTED_BIT | BASALT_WIFI_FAIL_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );
    s_wifi_connecting = false;

    if (bits & BASALT_WIFI_CONNECTED_BIT) {
        basalt_printf("wifi reconnect: connected to %s\n", s_wifi_ssid);
        bsh_cmd_wifi_status();
    } else if (bits & BASALT_WIFI_FAIL_BIT) {
        basalt_printf("wifi reconnect: failed to reconnect to %s\n", s_wifi_ssid);
    } else {
        basalt_printf("wifi reconnect: timeout waiting for connection\n");
    }
}

static void bsh_cmd_wifi_disconnect(void) {
    if (!s_wifi_ready) {
        basalt_printf("wifi disconnect: not initialized\n");
        return;
    }
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        basalt_printf("wifi disconnect: failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    s_wifi_connecting = false;
    s_wifi_connected = false;
    memset(&s_wifi_last_ip, 0, sizeof(s_wifi_last_ip));
    xEventGroupClearBits(s_wifi_events, BASALT_WIFI_CONNECTED_BIT | BASALT_WIFI_FAIL_BIT);
    basalt_printf("wifi disconnect: disconnected\n");
}

#else
static void bsh_cmd_wifi_status(void) {
    basalt_printf("wifi.enabled: %s\n", BASALT_ENABLE_WIFI ? "yes" : "no");
#if !BASALT_ENABLE_WIFI
    basalt_printf("wifi.runtime: disabled by driver selection\n");
#else
    basalt_printf("wifi.runtime: unavailable (CONFIG_ESP_WIFI_ENABLED=0)\n");
#endif
}
static void bsh_cmd_wifi_connect(const char *ssid, const char *pass) {
    (void)ssid;
    (void)pass;
    basalt_printf("wifi connect: Wi-Fi is not enabled in this firmware build/runtime\n");
}
static void bsh_cmd_wifi_scan(void) {
    basalt_printf("wifi scan: Wi-Fi is not enabled in this firmware build/runtime\n");
}
static void bsh_cmd_wifi_reconnect(void) {
    basalt_printf("wifi reconnect: Wi-Fi is not enabled in this firmware build/runtime\n");
}
static void bsh_cmd_wifi_disconnect(void) {
    basalt_printf("wifi disconnect: Wi-Fi is not enabled in this firmware build/runtime\n");
}
#endif

static void bsh_cmd_bluetooth_status(void);
static void bsh_cmd_bluetooth_on(void);
static void bsh_cmd_bluetooth_off(void);
static void bsh_cmd_bluetooth_scan(const char *seconds_arg);

#if BASALT_ENABLE_BLUETOOTH && defined(CONFIG_BT_ENABLED) && CONFIG_BT_ENABLED
#define BASALT_BT_SCAN_DONE_BIT BIT0
#define BASALT_BT_SCAN_PARAM_OK_BIT BIT1
#define BASALT_BT_SCAN_PARAM_FAIL_BIT BIT2

static EventGroupHandle_t s_bt_events = NULL;
static bool s_bt_gap_cb_registered = false;
static bool s_bt_ready = false;
static bool s_bt_scanning = false;
static uint16_t s_bt_scan_seen = 0;

static const esp_ble_scan_params_t s_bt_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

static void bt_bda_to_str(const uint8_t bda[6], char *out, size_t out_len) {
    if (!out || out_len < 18) return;
    snprintf(out, out_len, "%02X:%02X:%02X:%02X:%02X:%02X",
             bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}

static void bt_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (!param) return;
    if (event == ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT) {
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            if (s_bt_events) xEventGroupSetBits(s_bt_events, BASALT_BT_SCAN_PARAM_OK_BIT);
        } else {
            if (s_bt_events) xEventGroupSetBits(s_bt_events, BASALT_BT_SCAN_PARAM_FAIL_BIT);
        }
        return;
    }
    if (event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
        esp_ble_gap_cb_param_t *r = param;
        if (r->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            s_bt_scan_seen++;
            char addr[18] = {0};
            bt_bda_to_str(r->scan_rst.bda, addr, sizeof(addr));
            uint8_t name_len = 0;
            uint8_t *name = esp_ble_resolve_adv_data(r->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &name_len);
            if (!name || name_len == 0) {
                name = esp_ble_resolve_adv_data(r->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT, &name_len);
            }
            if (name && name_len > 0) {
                if (name_len > 29) name_len = 29;
                char nbuf[32];
                memcpy(nbuf, name, name_len);
                nbuf[name_len] = '\0';
                basalt_printf("bt %03u) %s rssi=%d name=%s\n",
                              (unsigned)s_bt_scan_seen, addr, r->scan_rst.rssi, nbuf);
            } else {
                basalt_printf("bt %03u) %s rssi=%d\n",
                              (unsigned)s_bt_scan_seen, addr, r->scan_rst.rssi);
            }
        } else if (r->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            s_bt_scanning = false;
            if (s_bt_events) xEventGroupSetBits(s_bt_events, BASALT_BT_SCAN_DONE_BIT);
        }
    }
}

static bool bt_stack_init(char *err, size_t err_len) {
    if (s_bt_ready) return true;
    if (!s_bt_events) {
        s_bt_events = xEventGroupCreate();
        if (!s_bt_events) {
            if (err && err_len) snprintf(err, err_len, "event group allocation failed");
            return false;
        }
    }

    esp_bt_controller_status_t ctl = esp_bt_controller_get_status();
    esp_err_t ret;
    if (ctl == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
        esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ret = esp_bt_controller_init(&cfg);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "controller init failed (%s)", esp_err_to_name(ret));
            return false;
        }
        ctl = esp_bt_controller_get_status();
    }
    if (ctl != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "controller enable failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        ret = esp_bluedroid_init();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "bluedroid init failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ret = esp_bluedroid_enable();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "bluedroid enable failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }

    if (!s_bt_gap_cb_registered) {
        ret = esp_ble_gap_register_callback(bt_gap_cb);
        if (ret != ESP_OK) {
            if (err && err_len) snprintf(err, err_len, "gap callback register failed (%s)", esp_err_to_name(ret));
            return false;
        }
        s_bt_gap_cb_registered = true;
    }

    s_bt_ready = true;
    return true;
}

static bool bt_stack_off(char *err, size_t err_len) {
    if (!s_bt_ready && esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        return true;
    }
    if (s_bt_scanning) {
        esp_ble_gap_stop_scanning();
        s_bt_scanning = false;
    }
    esp_err_t ret;
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        ret = esp_bluedroid_disable();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "bluedroid disable failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        ret = esp_bluedroid_deinit();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "bluedroid deinit failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }

    esp_bt_controller_status_t ctl = esp_bt_controller_get_status();
    if (ctl == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ret = esp_bt_controller_disable();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "controller disable failed (%s)", esp_err_to_name(ret));
            return false;
        }
        ctl = esp_bt_controller_get_status();
    }
    if (ctl == ESP_BT_CONTROLLER_STATUS_INITED) {
        ret = esp_bt_controller_deinit();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            if (err && err_len) snprintf(err, err_len, "controller deinit failed (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    s_bt_ready = false;
    return true;
}

static void bsh_cmd_bluetooth_status(void) {
    basalt_printf("bluetooth.enabled: %s\n", BASALT_ENABLE_BLUETOOTH ? "yes" : "no");
#if !BASALT_ENABLE_BLUETOOTH
    basalt_printf("bluetooth.runtime: disabled by driver selection\n");
#elif defined(CONFIG_BT_ENABLED) && CONFIG_BT_ENABLED
    basalt_printf("bluetooth.runtime: available (CONFIG_BT_ENABLED=1)\n");
    basalt_printf("bluetooth.state: %s\n", s_bt_ready ? "on" : "off");
    basalt_printf("bluetooth.scanning: %s\n", s_bt_scanning ? "yes" : "no");
#else
    basalt_printf("bluetooth.runtime: unavailable (CONFIG_BT_ENABLED=0)\n");
#endif
    basalt_printf("bluetooth.note: shell supports status/on/off/scan; pairing/GATT tooling is next\n");
}

static void bsh_cmd_bluetooth_on(void) {
    char err[128];
    if (!bt_stack_init(err, sizeof(err))) {
        basalt_printf("bluetooth on: %s\n", err);
        return;
    }
    basalt_printf("bluetooth on: ready (BLE stack enabled)\n");
}

static void bsh_cmd_bluetooth_off(void) {
    char err[128];
    if (!bt_stack_off(err, sizeof(err))) {
        basalt_printf("bluetooth off: %s\n", err);
        return;
    }
    basalt_printf("bluetooth off: stack disabled\n");
}

static void bsh_cmd_bluetooth_scan(const char *seconds_arg) {
    int seconds = 5;
    if (seconds_arg && seconds_arg[0]) {
        seconds = (int)strtol(seconds_arg, NULL, 10);
    }
    if (seconds < 1) seconds = 1;
    if (seconds > 30) seconds = 30;

    char err[128];
    if (!bt_stack_init(err, sizeof(err))) {
        basalt_printf("bluetooth scan: %s\n", err);
        return;
    }
    if (!s_bt_events) {
        basalt_printf("bluetooth scan: internal event group unavailable\n");
        return;
    }
    if (s_bt_scanning) {
        basalt_printf("bluetooth scan: already in progress\n");
        return;
    }

    xEventGroupClearBits(s_bt_events,
                         BASALT_BT_SCAN_DONE_BIT |
                         BASALT_BT_SCAN_PARAM_OK_BIT |
                         BASALT_BT_SCAN_PARAM_FAIL_BIT);
    s_bt_scan_seen = 0;
    esp_err_t ret = esp_ble_gap_set_scan_params((esp_ble_scan_params_t *)&s_bt_scan_params);
    if (ret != ESP_OK) {
        basalt_printf("bluetooth scan: set params failed (%s)\n", esp_err_to_name(ret));
        return;
    }

    EventBits_t pbits = xEventGroupWaitBits(
        s_bt_events,
        BASALT_BT_SCAN_PARAM_OK_BIT | BASALT_BT_SCAN_PARAM_FAIL_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(3000)
    );
    if (pbits & BASALT_BT_SCAN_PARAM_FAIL_BIT) {
        basalt_printf("bluetooth scan: controller rejected scan params\n");
        return;
    }
    if (!(pbits & BASALT_BT_SCAN_PARAM_OK_BIT)) {
        basalt_printf("bluetooth scan: timeout waiting for scan params\n");
        return;
    }

    ret = esp_ble_gap_start_scanning((uint32_t)seconds);
    if (ret != ESP_OK) {
        basalt_printf("bluetooth scan: start failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    s_bt_scanning = true;
    basalt_printf("bluetooth scan: scanning for %d second(s)...\n", seconds);

    EventBits_t bits = xEventGroupWaitBits(
        s_bt_events,
        BASALT_BT_SCAN_DONE_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS((seconds + 2) * 1000)
    );
    if (bits & BASALT_BT_SCAN_DONE_BIT) {
        basalt_printf("bluetooth scan: done, devices=%u\n", (unsigned)s_bt_scan_seen);
    } else {
        s_bt_scanning = false;
        basalt_printf("bluetooth scan: timeout waiting for completion\n");
    }
}

#else
static void bsh_cmd_bluetooth_status(void) {
    basalt_printf("bluetooth.enabled: %s\n", BASALT_ENABLE_BLUETOOTH ? "yes" : "no");
#if !BASALT_ENABLE_BLUETOOTH
    basalt_printf("bluetooth.runtime: disabled by driver selection\n");
#else
    basalt_printf("bluetooth.runtime: unavailable (CONFIG_BT_ENABLED=0)\n");
#endif
    basalt_printf("bluetooth.note: enable bluetooth driver and runtime support in firmware config\n");
}

static void bsh_cmd_bluetooth_on(void) {
    basalt_printf("bluetooth on: Bluetooth is not enabled in this firmware build/runtime\n");
}

static void bsh_cmd_bluetooth_off(void) {
    basalt_printf("bluetooth off: Bluetooth is not enabled in this firmware build/runtime\n");
}

static void bsh_cmd_bluetooth_scan(const char *seconds_arg) {
    (void)seconds_arg;
    basalt_printf("bluetooth scan: Bluetooth is not enabled in this firmware build/runtime\n");
}
#endif

#if BASALT_ENABLE_TWAI && SOC_TWAI_SUPPORTED
static bool s_twai_installed = false;
static bool s_twai_started = false;

static bool twai_parse_u32(const char *s, uint32_t *out) {
    if (!s || !*s || !out) return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (!end || *end != '\0') return false;
    *out = (uint32_t)v;
    return true;
}

static int twai_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool twai_parse_data_hex(const char *hex, uint8_t *out, uint8_t *out_len) {
    if (!hex || !out || !out_len) return false;
    size_t n = strlen(hex);
    if ((n % 2) != 0 || n == 0 || n > 16) return false;
    size_t bytes = n / 2;
    for (size_t i = 0; i < bytes; ++i) {
        int hi = twai_hex_nibble(hex[i * 2]);
        int lo = twai_hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    *out_len = (uint8_t)bytes;
    return true;
}

static const twai_timing_config_t *twai_pick_timing(void) {
    switch ((int)BASALT_CFG_TWAI_BITRATE) {
        case 125000: {
            static const twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
            return &t;
        }
        case 250000: {
            static const twai_timing_config_t t = TWAI_TIMING_CONFIG_250KBITS();
            return &t;
        }
        case 1000000: {
            static const twai_timing_config_t t = TWAI_TIMING_CONFIG_1MBITS();
            return &t;
        }
        case 500000:
        default: {
            static const twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
            return &t;
        }
    }
}

static bool basalt_twai_install(char *err, size_t err_len) {
    if (s_twai_installed) return true;
    if (BASALT_PIN_CAN_TX < 0 || BASALT_PIN_CAN_RX < 0) {
        if (err && err_len) snprintf(err, err_len, "invalid pins tx=%d rx=%d", BASALT_PIN_CAN_TX, BASALT_PIN_CAN_RX);
        return false;
    }
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)BASALT_PIN_CAN_TX, (gpio_num_t)BASALT_PIN_CAN_RX, TWAI_MODE_NORMAL);
    g.tx_queue_len = 10;
    g.rx_queue_len = 10;
    g.alerts_enabled = TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_RX_DATA | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF;
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    const twai_timing_config_t *t = twai_pick_timing();
    esp_err_t ret = twai_driver_install(&g, t, &f);
    if (ret != ESP_OK) {
        if (err && err_len) snprintf(err, err_len, "driver install failed (%s)", esp_err_to_name(ret));
        return false;
    }
    s_twai_installed = true;
    return true;
}

static void bsh_cmd_can_status(void) {
    basalt_printf("can.enabled: yes\n");
    basalt_printf("can.pins: tx=%d rx=%d\n", BASALT_PIN_CAN_TX, BASALT_PIN_CAN_RX);
    basalt_printf("can.bitrate: %d\n", (int)BASALT_CFG_TWAI_BITRATE);
    basalt_printf("can.installed: %s\n", s_twai_installed ? "yes" : "no");
    basalt_printf("can.started: %s\n", s_twai_started ? "yes" : "no");
    if (!s_twai_installed) return;
    twai_status_info_t st = {0};
    if (twai_get_status_info(&st) == ESP_OK) {
        basalt_printf("can.state: %d\n", (int)st.state);
        basalt_printf("can.tx_queued: %u\n", (unsigned)st.msgs_to_tx);
        basalt_printf("can.rx_queued: %u\n", (unsigned)st.msgs_to_rx);
        basalt_printf("can.tx_err: %u\n", (unsigned)st.tx_error_counter);
        basalt_printf("can.rx_err: %u\n", (unsigned)st.rx_error_counter);
        basalt_printf("can.bus_errors: %u\n", (unsigned)st.bus_error_count);
    }
}

static void bsh_cmd_can_up(void) {
    char err[128];
    if (!basalt_twai_install(err, sizeof(err))) {
        basalt_printf("can up: %s\n", err);
        return;
    }
    if (s_twai_started) {
        basalt_printf("can up: already running\n");
        return;
    }
    esp_err_t ret = twai_start();
    if (ret != ESP_OK) {
        basalt_printf("can up: failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    s_twai_started = true;
    basalt_printf("can up: running @ %d bps\n", (int)BASALT_CFG_TWAI_BITRATE);
}

static void bsh_cmd_can_down(void) {
    if (!s_twai_installed) {
        basalt_printf("can down: driver not installed\n");
        return;
    }
    if (!s_twai_started) {
        basalt_printf("can down: already stopped\n");
        return;
    }
    esp_err_t ret = twai_stop();
    if (ret != ESP_OK) {
        basalt_printf("can down: failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    s_twai_started = false;
    basalt_printf("can down: stopped\n");
}

static void bsh_cmd_can_send(const char *id_arg, const char *hex_arg) {
    if (!id_arg || !hex_arg) {
        basalt_printf("usage: can send <id> <hex>\n");
        basalt_printf("example: can send 0x123 DEADBEEF\n");
        return;
    }
    if (!s_twai_started) {
        basalt_printf("can send: bus not running (use 'can up')\n");
        return;
    }
    uint32_t id = 0;
    if (!twai_parse_u32(id_arg, &id)) {
        basalt_printf("can send: invalid id '%s'\n", id_arg);
        return;
    }
    uint8_t data[8] = {0};
    uint8_t dlc = 0;
    if (!twai_parse_data_hex(hex_arg, data, &dlc)) {
        basalt_printf("can send: invalid data hex '%s' (1..8 bytes, hex pairs)\n", hex_arg);
        return;
    }
    twai_message_t msg = {0};
    msg.identifier = id;
    msg.extd = (id > TWAI_STD_ID_MASK) ? 1 : 0;
    msg.data_length_code = dlc;
    memcpy(msg.data, data, dlc);
    esp_err_t ret = twai_transmit(&msg, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        basalt_printf("can send: failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    basalt_printf("can send: ok id=0x%lX dlc=%u\n", (unsigned long)id, (unsigned)dlc);
}

static void bsh_cmd_can_recv(const char *timeout_arg) {
    if (!s_twai_started) {
        basalt_printf("can recv: bus not running (use 'can up')\n");
        return;
    }
    int timeout_ms = 250;
    if (timeout_arg && timeout_arg[0]) {
        timeout_ms = (int)strtol(timeout_arg, NULL, 10);
        if (timeout_ms < 0) timeout_ms = 0;
        if (timeout_ms > 10000) timeout_ms = 10000;
    }
    twai_message_t msg = {0};
    esp_err_t ret = twai_receive(&msg, pdMS_TO_TICKS(timeout_ms));
    if (ret == ESP_ERR_TIMEOUT) {
        basalt_printf("can recv: timeout\n");
        return;
    }
    if (ret != ESP_OK) {
        basalt_printf("can recv: failed (%s)\n", esp_err_to_name(ret));
        return;
    }
    basalt_printf("can recv: id=0x%lX %s dlc=%u data=",
        (unsigned long)msg.identifier,
        msg.extd ? "ext" : "std",
        (unsigned)msg.data_length_code);
    for (uint8_t i = 0; i < msg.data_length_code && i < 8; ++i) {
        basalt_printf("%02X", msg.data[i]);
    }
    basalt_printf("\n");
}

#else
static void bsh_cmd_can_status(void) {
    basalt_printf("can.enabled: %s\n", BASALT_ENABLE_TWAI ? "yes" : "no");
#if !BASALT_ENABLE_TWAI
    basalt_printf("can.runtime: disabled by driver selection\n");
#elif !SOC_TWAI_SUPPORTED
    basalt_printf("can.runtime: unsupported on this target\n");
#else
    basalt_printf("can.runtime: unavailable\n");
#endif
}

static void bsh_cmd_can_up(void) {
    basalt_printf("can up: TWAI/CAN is not enabled in this firmware build/runtime\n");
}

static void bsh_cmd_can_down(void) {
    basalt_printf("can down: TWAI/CAN is not enabled in this firmware build/runtime\n");
}

static void bsh_cmd_can_send(const char *id_arg, const char *hex_arg) {
    (void)id_arg;
    (void)hex_arg;
    basalt_printf("can send: TWAI/CAN is not enabled in this firmware build/runtime\n");
}

static void bsh_cmd_can_recv(const char *timeout_arg) {
    (void)timeout_arg;
    basalt_printf("can recv: TWAI/CAN is not enabled in this firmware build/runtime\n");
}
#endif
#endif // BASALT_SHELL_LEVEL >= 3

static void bsh_cmd_applet_list(void) {
    basalt_printf("applets.count: %d\n", (int)BASALT_APPLET_COUNT);
    const char *csv = BASALT_APPLETS_CSV;
    if (!csv || csv[0] == '\0') {
        basalt_printf("applets: (none selected)\n");
        return;
    }

    basalt_printf("applets.selected:\n");
    int idx = 1;
    const char *start = csv;
    const char *p = csv;
    while (true) {
        if (*p == ',' || *p == '\0') {
            int len = (int)(p - start);
            if (len > 0) {
                char name[48];
                if (len >= (int)sizeof(name)) len = (int)sizeof(name) - 1;
                memcpy(name, start, (size_t)len);
                name[len] = '\0';
                basalt_printf("  %d) %s\n", idx++, name);
            }
            if (*p == '\0') break;
            start = p + 1;
        }
        ++p;
    }
}

static void bsh_cmd_applet_run(const char *name) {
    if (!name || !name[0]) {
        basalt_printf("usage: applet run <name>\n");
        return;
    }
    char script[256];
    if (!resolve_named_app_script(name, script, sizeof(script))) {
        basalt_printf("applet run: no entry script in /apps/%s\n", name);
        return;
    }

    char err[128];
    basalt_printf("applet run: launching %s\n", script);
    if (!mpy_runtime_start_file(script, err, sizeof(err))) {
        basalt_printf("applet run: %s\n", err);
    }
}

#if BASALT_SHELL_LEVEL == 1
static bool bsh_tiny_is_blocked(const char *cmd) {
    static const char *k_blocked[] = {
        "ls", "cat", "cd", "mkdir", "cp", "mv", "rm",
        "apps_dev", "led_test", "devcheck", "edit",
        "run_dev", "kill", "applet", "applets",
        "install", "remove", "logs", "imu", "dht22", "ads1115", "mcp23017", "tp4056", "mcp2544fd", "uln2003", "l298n", "wifi", "bluetooth", "bt", "can"
    };
    for (size_t i = 0; i < sizeof(k_blocked) / sizeof(k_blocked[0]); ++i) {
        if (strcmp(cmd, k_blocked[i]) == 0) return true;
    }
    return false;
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
            basalt_printf("help: unknown command '%s'\n", arg);
            basalt_printf("hint: use 'help -commands'\n");
        }
        return;
    }
#if BASALT_SHELL_LEVEL == 1
    if (bsh_tiny_is_blocked(cmd)) {
        basalt_printf("%s: disabled in tiny shell\n", cmd);
        return;
    }
#endif
    else if (strcmp(cmd, "ls") == 0) {
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
#if BASALT_SHELL_LEVEL >= 3
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_mkdir(arg);
#else
        basalt_printf("mkdir: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "cp") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_cp(src, dst, recursive);
#else
        basalt_printf("cp: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "mv") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        bool recursive = false;
        char *src = strtok(NULL, " \t\r\n");
        if (src && (strcmp(src, "-r") == 0 || strcmp(src, "-R") == 0)) {
            recursive = true;
            src = strtok(NULL, " \t\r\n");
        }
        char *dst = strtok(NULL, " \t\r\n");
        bsh_cmd_mv(src, dst, recursive);
#else
        basalt_printf("mv: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "rm") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *arg = strtok(NULL, " \t\r\n");
        bool recursive = false;
        if (arg && strcmp(arg, "-r") == 0) {
            recursive = true;
            arg = strtok(NULL, " \t\r\n");
        }
        bsh_cmd_rm(arg, recursive);
#else
        basalt_printf("rm: disabled in this shell level\n");
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
    } else if (strcmp(cmd, "drivers") == 0) {
        bsh_cmd_drivers();
    } else if (strcmp(cmd, "edit") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *arg = strtok(NULL, " \t\r\n");
        bsh_cmd_edit(arg);
#else
        basalt_printf("edit: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "run") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        if (!arg) {
            basalt_printf("run: missing required argument <app|script>\n");
            bsh_print_usage("run");
        } else {
            const bool is_name = !path_is_absolute(arg) && !strchr(arg, '/') && !strstr(arg, ".py");
            if (is_name) {
                char script[256];
                if (!resolve_named_app_script(arg, script, sizeof(script))) {
                    basalt_printf("run: no entry script in /apps/%s\n", arg);
                    return;
                }
                char err[128];
                basalt_printf("run: launching %s\n", script);
                if (!mpy_runtime_start_file(script, err, sizeof(err))) {
                    basalt_printf("run: %s\n", err);
                }
                return;
            }
            char vpath[128];
            resolve_virtual_path(arg, vpath, sizeof(vpath));
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
            basalt_printf("run: launching %s\n", script);
            if (!mpy_runtime_start_file(script, err, sizeof(err))) {
                basalt_printf("run: %s\n", err);
            }
        }
    } else if (strcmp(cmd, "run_dev") == 0) {
        char *arg = strtok(NULL, " \t\r\n");
        if (!arg) {
            basalt_printf("run_dev: missing required argument <app|script>\n");
            bsh_print_usage("run_dev");
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
    } else if (strcmp(cmd, "applet") == 0 || strcmp(cmd, "applets") == 0) {
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "list") == 0) {
            bsh_cmd_applet_list();
        } else if (strcmp(sub, "run") == 0) {
            char *name = strtok(NULL, " \t\r\n");
            bsh_cmd_applet_run(name);
        } else {
            bsh_print_unknown_subcommand("applet", sub);
        }
    } else if (strcmp(cmd, "install") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *src = strtok(NULL, " \t\r\n");
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_install(src, name);
#else
        basalt_printf("install: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "remove") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *name = strtok(NULL, " \t\r\n");
        bsh_cmd_remove(name);
#else
        basalt_printf("remove: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "logs") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        const char *current = mpy_runtime_current_app();
        const char *last_err = mpy_runtime_last_error();
        const char *last_result = mpy_runtime_last_result();
        basalt_printf("runtime.ready: %s\n", mpy_runtime_is_ready() ? "yes" : "no");
        basalt_printf("runtime.running: %s\n", mpy_runtime_is_running() ? "yes" : "no");
        basalt_printf("runtime.app: %s\n", current ? current : "(none)");
        basalt_printf("runtime.last_result: %s\n", last_result ? last_result : "(none)");
        basalt_printf("runtime.last_error: %s\n", last_err ? last_err : "(none)");
        basalt_printf("runtime.history_persistence: RAM-only (clears on reboot)\n");
        basalt_printf("system.heap_free: %u\n", (unsigned)esp_get_free_heap_size());
        basalt_printf("system.cpu_hz: %u\n", (unsigned)esp_clk_cpu_freq());
        basalt_printf("hint: use idf.py monitor for full ESP-IDF logs\n");
#else
        basalt_printf("logs: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "bme280") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0 || strcmp(sub, "probe") == 0) {
            bsh_cmd_bme280_status();
        } else if (strcmp(sub, "read") == 0) {
            bsh_cmd_bme280_read();
        } else {
            bsh_print_unknown_subcommand("bme280", sub);
        }
#else
        basalt_printf("bme280: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "dht22") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0 || strcmp(sub, "probe") == 0) {
            bsh_cmd_dht22_status();
        } else if (strcmp(sub, "read") == 0) {
            bsh_cmd_dht22_read(arg1, arg2);
        } else {
            bsh_print_unknown_subcommand("dht22", sub);
        }
#else
        basalt_printf("dht22: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "ads1115") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0 || strcmp(sub, "probe") == 0) {
            bsh_cmd_ads1115_status();
        } else if (strcmp(sub, "read") == 0) {
            bsh_cmd_ads1115_read(arg1);
        } else {
            bsh_print_unknown_subcommand("ads1115", sub);
        }
#else
        basalt_printf("ads1115: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "mcp23017") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        bsh_cmd_mcp23017(sub, arg1, arg2);
#else
        basalt_printf("mcp23017: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "imu") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0) {
            bsh_cmd_imu_status();
        } else if (strcmp(sub, "read") == 0) {
            bsh_cmd_imu_read();
        } else if (strcmp(sub, "whoami") == 0) {
            bsh_cmd_imu_whoami();
        } else if (strcmp(sub, "stream") == 0) {
            char *hz = strtok(NULL, " \t\r\n");
            char *samples = strtok(NULL, " \t\r\n");
            bsh_cmd_imu_stream(hz, samples);
        } else {
            bsh_print_unknown_subcommand("imu", sub);
        }
#else
        basalt_printf("imu: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "tp4056") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        bsh_cmd_tp4056(sub);
#else
        basalt_printf("tp4056: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "mcp2544fd") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        bsh_cmd_mcp2544fd(sub);
#else
        basalt_printf("mcp2544fd: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "mcp2515") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        bsh_cmd_mcp2515(sub, arg1, arg2);
#else
        basalt_printf("mcp2515: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "uln2003") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        bsh_cmd_uln2003(sub, arg1, arg2);
#else
        basalt_printf("uln2003: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "l298n") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        bsh_cmd_l298n(sub, arg1, arg2);
#else
        basalt_printf("l298n: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "wifi") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0) {
            bsh_cmd_wifi_status();
        } else if (strcmp(sub, "scan") == 0) {
            bsh_cmd_wifi_scan();
        } else if (strcmp(sub, "connect") == 0) {
            char *ssid = strtok(NULL, " \t\r\n");
            char *pass = strtok(NULL, " \t\r\n");
            bsh_cmd_wifi_connect(ssid, pass);
        } else if (strcmp(sub, "reconnect") == 0) {
            bsh_cmd_wifi_reconnect();
        } else if (strcmp(sub, "disconnect") == 0) {
            bsh_cmd_wifi_disconnect();
        } else {
            bsh_print_unknown_subcommand("wifi", sub);
        }
#else
        basalt_printf("wifi: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "bluetooth") == 0 || strcmp(cmd, "bt") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0) {
            bsh_cmd_bluetooth_status();
        } else if (strcmp(sub, "on") == 0) {
            bsh_cmd_bluetooth_on();
        } else if (strcmp(sub, "off") == 0) {
            bsh_cmd_bluetooth_off();
        } else if (strcmp(sub, "scan") == 0) {
            char *seconds = strtok(NULL, " \t\r\n");
            bsh_cmd_bluetooth_scan(seconds);
        } else {
            bsh_print_unknown_subcommand("bluetooth", sub);
        }
#else
        basalt_printf("bluetooth: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "can") == 0) {
#if BASALT_SHELL_LEVEL >= 3
        char *sub = strtok(NULL, " \t\r\n");
        if (!sub || strcmp(sub, "status") == 0) {
            bsh_cmd_can_status();
        } else if (strcmp(sub, "up") == 0) {
            bsh_cmd_can_up();
        } else if (strcmp(sub, "down") == 0) {
            bsh_cmd_can_down();
        } else if (strcmp(sub, "send") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            char *hex = strtok(NULL, " \t\r\n");
            bsh_cmd_can_send(id, hex);
        } else if (strcmp(sub, "recv") == 0) {
            char *timeout = strtok(NULL, " \t\r\n");
            bsh_cmd_can_recv(timeout);
        } else {
            bsh_print_unknown_subcommand("can", sub);
        }
#else
        basalt_printf("can: disabled in this shell level\n");
#endif
    } else if (strcmp(cmd, "reboot") == 0) {
        basalt_printf("rebooting...\n");
        fflush(stdout);
        esp_restart();
    } else {
        bsh_print_unknown_command(cmd);
    }
}

static int basalt_uart_readline(char *out, size_t out_len) {
    if (!out || out_len == 0) return 0;
    size_t n = 0;
    while (n + 1 < out_len) {
        uint8_t ch = 0;
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED) && CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
        if (!usb_serial_jtag_is_driver_installed()) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        int got = usb_serial_jtag_read_bytes(&ch, 1, 20 / portTICK_PERIOD_MS);
        if (got <= 0) {
            continue;
        }
#else
        int c = getchar();
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        ch = (uint8_t)c;
#endif
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
    g_uart_num = CONFIG_ESP_CONSOLE_UART_NUM;
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED) && CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t usj = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        esp_err_t err = usb_serial_jtag_driver_install(&usj);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Console: usb_serial_jtag install failed (%s), using stdio fallback", esp_err_to_name(err));
        }
    }
    ESP_LOGI(TAG, "Console: usb_serial_jtag backend active");
#else
    ESP_LOGI(TAG, "Console: stdio backend active");
#endif
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

static void basalt_log_driver_boot_summary(void) {
#if BASALT_ENABLE_DISPLAY_SSD1306 || BASALT_ENABLE_RTC || BASALT_ENABLE_IMU || BASALT_ENABLE_DHT22 || BASALT_ENABLE_BME280 || BASALT_ENABLE_ADS1115 || BASALT_ENABLE_MCP23017 || BASALT_ENABLE_MIC || BASALT_ENABLE_TP4056 || BASALT_ENABLE_TWAI || BASALT_ENABLE_MCP2544FD || BASALT_ENABLE_ULN2003 || BASALT_ENABLE_L298N
    ESP_LOGW(TAG, "Experimental drivers enabled; some are config-only stubs in this build.");
#endif
#if BASALT_ENABLE_DISPLAY_SSD1306
    ESP_LOGI(TAG, "display_ssd1306 enabled: MicroPython API available via basalt.ssd1306.");
#endif
#if BASALT_ENABLE_RTC
    ESP_LOGI(TAG, "rtc enabled: MicroPython API available via basalt.rtc.");
#endif
#if BASALT_ENABLE_IMU
    ESP_LOGI(TAG, "imu enabled: shell IMU command available (imu status/read/whoami).");
#endif
#if BASALT_ENABLE_BME280
    ESP_LOGI(TAG, "bme280 enabled: shell API available (bme280 status/probe/read).");
#endif
#if BASALT_ENABLE_ADS1115
    ESP_LOGI(TAG, "ads1115 enabled: shell API available (ads1115 status/probe/read).");
#endif
#if BASALT_ENABLE_MCP23017
    ESP_LOGI(TAG, "mcp23017 enabled: shell API available (mcp23017 status/probe/scan/test/read/write/pinmode/pinwrite/pinread).");
#endif
#if BASALT_ENABLE_DHT22
    ESP_LOGI(TAG, "dht22 enabled: shell API available (dht22 status/read [pin] [auto|dht22|dht11]).");
#endif
#if BASALT_ENABLE_MIC
    ESP_LOGW(TAG, "mic enabled: runtime driver not implemented yet.");
#endif
#if BASALT_ENABLE_TP4056
    ESP_LOGI(TAG, "tp4056 enabled: shell API available (tp4056 status/on/off).");
#endif
#if BASALT_ENABLE_TWAI
    ESP_LOGI(TAG, "twai enabled: shell API available (can status/up/down/send/recv).");
#endif
#if BASALT_ENABLE_MCP2544FD
    ESP_LOGI(TAG, "mcp2544fd enabled: shell API available (mcp2544fd status/on/off/standby).");
#endif
#if BASALT_ENABLE_ULN2003
    ESP_LOGI(TAG, "uln2003 enabled: shell API available (uln2003 status/off/step).");
#endif
#if BASALT_ENABLE_L298N
    ESP_LOGI(TAG, "l298n enabled: shell API available (l298n status/stop/test/speed/a/b).");
#endif
}

static void basalt_autorun_market_apps(void) {
#if defined(BASALT_MARKET_APP_ENABLED_FLAPPY_BIRD) && BASALT_MARKET_APP_ENABLED_FLAPPY_BIRD
    char script[256];
    if (!resolve_named_app_script("flappy_bird", script, sizeof(script))) {
        ESP_LOGW(TAG, "autorun flappy_bird skipped: app entry not found");
    } else {
        char err[128] = {0};
        ESP_LOGI(TAG, "autorun: launching %s", script);
        if (!mpy_runtime_start_file(script, err, sizeof(err))) {
            ESP_LOGE(TAG, "autorun flappy_bird failed: %s", err[0] ? err : "unknown error");
        }
    }
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
    basalt_log_driver_boot_summary();

    (void)basalt_smoke_test_run();
    basalt_autorun_market_apps();

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
