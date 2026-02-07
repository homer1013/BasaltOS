#include "py/obj.h"
#include "py/runtime.h"

#include "board_config.h"
#include "modui.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#ifndef BASALT_ENABLE_RTC
#define BASALT_ENABLE_RTC 0
#endif
#ifndef BASALT_ENABLE_I2C
#define BASALT_ENABLE_I2C 0
#endif
#ifndef BASALT_PIN_I2C_SDA
#define BASALT_PIN_I2C_SDA -1
#endif
#ifndef BASALT_PIN_I2C_SCL
#define BASALT_PIN_I2C_SCL -1
#endif
#ifndef BASALT_CFG_RTC_ADDRESS
#define BASALT_CFG_RTC_ADDRESS "0x68"
#endif
#ifndef BASALT_ENABLE_DISPLAY_SSD1306
#define BASALT_ENABLE_DISPLAY_SSD1306 0
#endif
#ifndef BASALT_DISPLAY_SSD1306_ADDRESS
#ifdef BASALT_CFG_DISPLAY_SSD1306_ADDRESS
#define BASALT_DISPLAY_SSD1306_ADDRESS BASALT_CFG_DISPLAY_SSD1306_ADDRESS
#else
#define BASALT_DISPLAY_SSD1306_ADDRESS "0x3C"
#endif
#endif
#ifndef BASALT_DISPLAY_SSD1306_WIDTH
#ifdef BASALT_CFG_DISPLAY_SSD1306_WIDTH
#define BASALT_DISPLAY_SSD1306_WIDTH BASALT_CFG_DISPLAY_SSD1306_WIDTH
#else
#define BASALT_DISPLAY_SSD1306_WIDTH 128
#endif
#endif
#ifndef BASALT_DISPLAY_SSD1306_HEIGHT
#ifdef BASALT_CFG_DISPLAY_SSD1306_HEIGHT
#define BASALT_DISPLAY_SSD1306_HEIGHT BASALT_CFG_DISPLAY_SSD1306_HEIGHT
#else
#define BASALT_DISPLAY_SSD1306_HEIGHT 64
#endif
#endif
#ifndef BASALT_DISPLAY_SSD1306_I2C_HZ
#ifdef BASALT_CFG_DISPLAY_SSD1306_I2C_HZ
#define BASALT_DISPLAY_SSD1306_I2C_HZ BASALT_CFG_DISPLAY_SSD1306_I2C_HZ
#else
#define BASALT_DISPLAY_SSD1306_I2C_HZ 400000
#endif
#endif

static const char *TAG = "modbasalt";

#if BASALT_ENABLE_I2C
static bool s_i2c0_ready = false;
static esp_err_t basalt_i2c0_ensure(int speed_hz) {
    if (s_i2c0_ready) {
        return ESP_OK;
    }
    if (BASALT_PIN_I2C_SDA < 0 || BASALT_PIN_I2C_SCL < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (speed_hz <= 0) speed_hz = 100000;

    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BASALT_PIN_I2C_SDA,
        .scl_io_num = BASALT_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = (uint32_t)speed_hz,
    };

    esp_err_t err = i2c_param_config(I2C_NUM_0, &cfg);
    if (err != ESP_OK) {
        return err;
    }
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        s_i2c0_ready = true;
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    s_i2c0_ready = true;
    return ESP_OK;
}
#endif

// basalt.gpio.mode(pin, mode) where mode: 0=input, 1=output
static mp_obj_t basalt_gpio_mode(mp_obj_t pin_obj, mp_obj_t mode_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int mode = mp_obj_get_int(mode_obj);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = (mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(basalt_gpio_mode_obj, basalt_gpio_mode);

// basalt.gpio.set(pin, value)
static mp_obj_t basalt_gpio_set(mp_obj_t pin_obj, mp_obj_t val_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int val = mp_obj_is_true(val_obj) ? 1 : 0;
    gpio_set_level(pin, val);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(basalt_gpio_set_obj, basalt_gpio_set);

// basalt.gpio.get(pin)
static mp_obj_t basalt_gpio_get(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int val = gpio_get_level(pin);
    return mp_obj_new_int(val);
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_gpio_get_obj, basalt_gpio_get);

// basalt.timer.sleep_ms(ms)
static mp_obj_t basalt_timer_sleep_ms(mp_obj_t ms_obj) {
    int ms = mp_obj_get_int(ms_obj);
    if (ms < 0) ms = 0;
    vTaskDelay(pdMS_TO_TICKS(ms));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_timer_sleep_ms_obj, basalt_timer_sleep_ms);

static void basalt_led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
#if BASALT_LED_R_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_R_PIN);
#endif
#if BASALT_LED_G_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_G_PIN);
#endif
#if BASALT_LED_B_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_B_PIN);
#endif
    if (io_conf.pin_bit_mask) {
        gpio_config(&io_conf);
    }
}

static void basalt_led_set_unique_pin(int pin, int r, int g, int b) {
    if (pin < 0) return;
    int want_on = 0;
    if (BASALT_LED_R_PIN >= 0 && BASALT_LED_R_PIN == pin) want_on |= r;
    if (BASALT_LED_G_PIN >= 0 && BASALT_LED_G_PIN == pin) want_on |= g;
    if (BASALT_LED_B_PIN >= 0 && BASALT_LED_B_PIN == pin) want_on |= b;
    int level = BASALT_LED_ACTIVE_LOW ? !want_on : want_on;
    gpio_set_level(pin, level);
}

// basalt.led.set(r, g, b)
static mp_obj_t basalt_led_set(mp_obj_t r_obj, mp_obj_t g_obj, mp_obj_t b_obj) {
    basalt_led_init();
    int r = mp_obj_is_true(r_obj) ? 1 : 0;
    int g = mp_obj_is_true(g_obj) ? 1 : 0;
    int b = mp_obj_is_true(b_obj) ? 1 : 0;

    // Handle boards where multiple channels map to the same physical pin.
    basalt_led_set_unique_pin(BASALT_LED_R_PIN, r, g, b);
    if (BASALT_LED_G_PIN != BASALT_LED_R_PIN) {
        basalt_led_set_unique_pin(BASALT_LED_G_PIN, r, g, b);
    }
    if (BASALT_LED_B_PIN != BASALT_LED_R_PIN && BASALT_LED_B_PIN != BASALT_LED_G_PIN) {
        basalt_led_set_unique_pin(BASALT_LED_B_PIN, r, g, b);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(basalt_led_set_obj, basalt_led_set);

// basalt.led.off()
static mp_obj_t basalt_led_off(void) {
    int off = BASALT_LED_ACTIVE_LOW ? 1 : 0;
    if (BASALT_LED_R_PIN >= 0) gpio_set_level(BASALT_LED_R_PIN, off);
    if (BASALT_LED_G_PIN >= 0) gpio_set_level(BASALT_LED_G_PIN, off);
    if (BASALT_LED_B_PIN >= 0) gpio_set_level(BASALT_LED_B_PIN, off);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_led_off_obj, basalt_led_off);

// ------------------------
// RTC helper API (DS3231-compatible over I2C)
// ------------------------
#if BASALT_ENABLE_RTC
static uint8_t basalt_rtc_address(void) {
    long addr = strtol(BASALT_CFG_RTC_ADDRESS, NULL, 0);
    if (addr <= 0 || addr > 0x7F) {
        addr = 0x68;
    }
    return (uint8_t)addr;
}

static inline uint8_t bcd_to_dec(uint8_t v) {
    return (uint8_t)(((v >> 4) * 10) + (v & 0x0F));
}

static esp_err_t basalt_rtc_i2c_ensure(void) {
#if !(BASALT_ENABLE_RTC && BASALT_ENABLE_I2C)
    return ESP_ERR_NOT_SUPPORTED;
#else
    return basalt_i2c0_ensure(100000);
#endif
}

static esp_err_t basalt_rtc_read_regs(uint8_t reg, uint8_t *buf, size_t len) {
    esp_err_t err = basalt_rtc_i2c_ensure();
    if (err != ESP_OK) {
        return err;
    }
    const uint8_t addr = basalt_rtc_address();
    return i2c_master_write_read_device(I2C_NUM_0, addr, &reg, 1, buf, len, pdMS_TO_TICKS(50));
}

// basalt.rtc.available()
static mp_obj_t basalt_rtc_available(void) {
#if BASALT_ENABLE_RTC && BASALT_ENABLE_I2C
    uint8_t sec = 0;
    esp_err_t err = basalt_rtc_read_regs(0x00, &sec, 1);
    return mp_obj_new_bool(err == ESP_OK);
#else
    return mp_const_false;
#endif
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_rtc_available_obj, basalt_rtc_available);

// basalt.rtc.now() -> (year, month, day, hour, minute, second, weekday)
static mp_obj_t basalt_rtc_now(void) {
#if !(BASALT_ENABLE_RTC && BASALT_ENABLE_I2C)
    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("rtc unavailable (enable rtc+i2c modules)"));
#else
    uint8_t regs[7] = {0};
    esp_err_t err = basalt_rtc_read_regs(0x00, regs, sizeof(regs));
    if (err != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("rtc read failed (%d)"), (int)err);
    }

    int second = bcd_to_dec(regs[0] & 0x7F);
    int minute = bcd_to_dec(regs[1] & 0x7F);

    int hour = 0;
    if (regs[2] & 0x40) {
        hour = bcd_to_dec(regs[2] & 0x1F);
        if (regs[2] & 0x20) {
            hour = (hour % 12) + 12;
        } else if (hour == 12) {
            hour = 0;
        }
    } else {
        hour = bcd_to_dec(regs[2] & 0x3F);
    }

    int weekday = bcd_to_dec(regs[3] & 0x07);
    int day = bcd_to_dec(regs[4] & 0x3F);
    int month = bcd_to_dec(regs[5] & 0x1F);
    int year = 2000 + bcd_to_dec(regs[6]);

    mp_obj_t out[7] = {
        mp_obj_new_int(year),
        mp_obj_new_int(month),
        mp_obj_new_int(day),
        mp_obj_new_int(hour),
        mp_obj_new_int(minute),
        mp_obj_new_int(second),
        mp_obj_new_int(weekday),
    };
    return mp_obj_new_tuple(7, out);
#endif
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_rtc_now_obj, basalt_rtc_now);

// basalt.rtc.address()
static mp_obj_t basalt_rtc_address_obj(void) {
    return mp_obj_new_int((int)basalt_rtc_address());
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_rtc_address_fun_obj, basalt_rtc_address_obj);
#endif // BASALT_ENABLE_RTC

#if BASALT_ENABLE_DISPLAY_SSD1306
static bool s_ssd_inited = false;
static int s_ssd_color = 1;
static uint8_t s_ssd_fb[1024];
static int s_ssd_text_x = 0;
static int s_ssd_text_y = 0;
static uint8_t s_ssd_addr = 0;

static int basalt_ssd_width(void) {
    int w = BASALT_DISPLAY_SSD1306_WIDTH;
    if (w < 1) w = 1;
    if (w > 128) w = 128;
    return w;
}

static int basalt_ssd_height(void) {
    int h = BASALT_DISPLAY_SSD1306_HEIGHT;
    if (h < 8) h = 8;
    if (h > 64) h = 64;
    // page-aligned rows
    h = (h / 8) * 8;
    return h;
}

static int basalt_ssd_fb_bytes(void) {
    return (basalt_ssd_width() * basalt_ssd_height()) / 8;
}

static uint8_t basalt_ssd_address_configured(void) {
    long addr = strtol(BASALT_DISPLAY_SSD1306_ADDRESS, NULL, 0);
    if (addr <= 0 || addr > 0x7F) {
        addr = 0x3C;
    }
    return (uint8_t)addr;
}

static uint8_t basalt_ssd_address(void) {
    return s_ssd_addr ? s_ssd_addr : basalt_ssd_address_configured();
}

static esp_err_t basalt_i2c_probe_addr(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) return ESP_ERR_NO_MEM;
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (uint8_t)((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(40));
    i2c_cmd_link_delete(cmd);
    return err;
}

static void basalt_i2c_log_scan(void) {
    char line[160];
    size_t off = 0;
    off += (size_t)snprintf(line + off, sizeof(line) - off, "ssd1306: i2c scan:");
    bool found = false;
    for (int a = 0x08; a <= 0x77; ++a) {
        if (basalt_i2c_probe_addr((uint8_t)a) == ESP_OK) {
            found = true;
            if (off < sizeof(line) - 6) {
                off += (size_t)snprintf(line + off, sizeof(line) - off, " 0x%02X", a);
            }
        }
    }
    if (!found) {
        ESP_LOGW(TAG, "ssd1306: i2c scan found no devices");
    } else {
        ESP_LOGW(TAG, "%s", line);
    }
}

static esp_err_t basalt_ssd_i2c_ensure(void) {
#if !BASALT_ENABLE_I2C
    ESP_LOGW(TAG, "ssd1306: I2C disabled at build time");
    return ESP_ERR_NOT_SUPPORTED;
#else
    int hz = BASALT_DISPLAY_SSD1306_I2C_HZ;
    if (hz < 100000) hz = 100000;
    if (hz > 400000) hz = 400000;
    esp_err_t err = basalt_i2c0_ensure(hz);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ssd1306: I2C ensure failed (%s)", esp_err_to_name(err));
    }
    return err;
#endif
}

static esp_err_t basalt_ssd_write_cmds(const uint8_t *cmds, size_t len) {
    if (!cmds || len == 0) return ESP_OK;
    uint8_t buf[17];
    size_t off = 0;
    while (off < len) {
        size_t n = len - off;
        if (n > 16) n = 16;
        buf[0] = 0x00;
        memcpy(&buf[1], &cmds[off], n);
        esp_err_t err = i2c_master_write_to_device(I2C_NUM_0, basalt_ssd_address(), buf, n + 1, pdMS_TO_TICKS(50));
        if (err != ESP_OK) return err;
        off += n;
    }
    return ESP_OK;
}

static esp_err_t basalt_ssd_write_data(const uint8_t *data, size_t len) {
    if (!data || len == 0) return ESP_OK;
    uint8_t buf[17];
    size_t off = 0;
    while (off < len) {
        size_t n = len - off;
        if (n > 16) n = 16;
        buf[0] = 0x40;
        memcpy(&buf[1], &data[off], n);
        esp_err_t err = i2c_master_write_to_device(I2C_NUM_0, basalt_ssd_address(), buf, n + 1, pdMS_TO_TICKS(50));
        if (err != ESP_OK) return err;
        off += n;
    }
    return ESP_OK;
}

static esp_err_t basalt_ssd_init(void) {
    if (s_ssd_inited) return ESP_OK;
    esp_err_t err = basalt_ssd_i2c_ensure();
    if (err != ESP_OK) return err;

    const uint8_t preferred = basalt_ssd_address_configured();
    const uint8_t alt = (preferred == 0x3C) ? 0x3D : 0x3C;
    if (basalt_i2c_probe_addr(preferred) == ESP_OK) {
        s_ssd_addr = preferred;
    } else if (basalt_i2c_probe_addr(alt) == ESP_OK) {
        s_ssd_addr = alt;
        ESP_LOGW(TAG, "ssd1306: configured addr 0x%02X not responding, using 0x%02X",
            preferred, alt);
    } else {
        s_ssd_addr = preferred;
        ESP_LOGW(TAG, "ssd1306: no device ACK at 0x%02X or 0x%02X", preferred, alt);
        basalt_i2c_log_scan();
    }

    const int h = basalt_ssd_height();
    const uint8_t com_pins = (h > 32) ? 0x12 : 0x02;
    const uint8_t seq[] = {
        0xAE, // display off
        0x20, 0x00, // horizontal addressing mode
        0x40, // display start line
        0xA1, // segment remap
        0xC8, // COM scan dec
        0x81, 0x8F, // contrast
        0xA8, (uint8_t)(h - 1), // multiplex ratio
        0xD3, 0x00, // display offset
        0xDA, com_pins, // COM pins
        0xD5, 0x80, // display clock
        0xD9, 0xF1, // pre-charge
        0xDB, 0x40, // VCOM detect
        0x8D, 0x14, // charge pump on
        0xA4, // display follows RAM
        0xA6, // normal display
        0x2E, // deactivate scroll
        0xAF, // display on
    };
    err = basalt_ssd_write_cmds(seq, sizeof(seq));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ssd1306: init command sequence failed addr=0x%02X (%s)",
            basalt_ssd_address(), esp_err_to_name(err));
        return err;
    }

    memset(s_ssd_fb, 0, sizeof(s_ssd_fb));
    s_ssd_inited = true;
    ESP_LOGI(TAG, "ssd1306: init OK addr=0x%02X %dx%d",
        basalt_ssd_address(), basalt_ssd_width(), basalt_ssd_height());
    return ESP_OK;
}

static void basalt_ssd_put_pixel(int x, int y, int color) {
    const int w = basalt_ssd_width();
    const int h = basalt_ssd_height();
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    const int idx = x + (y / 8) * w;
    const uint8_t bit = (uint8_t)(1u << (y & 7));
    if (color) s_ssd_fb[idx] |= bit;
    else s_ssd_fb[idx] &= (uint8_t)~bit;
}

static void basalt_ssd_hline(int x, int y, int len, int color) {
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) basalt_ssd_put_pixel(x + i, y, color);
}

static void basalt_ssd_vline(int x, int y, int len, int color) {
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) basalt_ssd_put_pixel(x, y + i, color);
}

static const uint8_t *basalt_ssd_debug_glyph(char ch) {
    // 5x7 debug glyph rows (bit4..bit0 are pixels left..right).
    static const uint8_t g_space[7] = {0,0,0,0,0,0,0};
    static const uint8_t g_dash[7]  = {0,0,0,31,0,0,0};
    static const uint8_t g_dot[7]   = {0,0,0,0,0,12,12};
    static const uint8_t g_colon[7] = {0,12,12,0,12,12,0};
    static const uint8_t g_slash[7] = {1,2,4,8,16,0,0};
    static const uint8_t g_qmark[7] = {14,17,1,2,4,0,4};
    static const uint8_t g_box[7]   = {31,17,17,17,17,17,31};

    static const uint8_t g_digits[10][7] = {
        {14,17,19,21,25,17,14}, // 0
        {4,12,4,4,4,4,14},      // 1
        {14,17,1,2,4,8,31},     // 2
        {30,1,1,14,1,1,30},     // 3
        {2,6,10,18,31,2,2},     // 4
        {31,16,16,30,1,1,30},   // 5
        {14,16,16,30,17,17,14}, // 6
        {31,1,2,4,8,8,8},       // 7
        {14,17,17,14,17,17,14}, // 8
        {14,17,17,15,1,1,14},   // 9
    };

    static const uint8_t g_upper[26][7] = {
        {14,17,17,31,17,17,17}, // A
        {30,17,17,30,17,17,30}, // B
        {14,17,16,16,16,17,14}, // C
        {30,17,17,17,17,17,30}, // D
        {31,16,16,30,16,16,31}, // E
        {31,16,16,30,16,16,16}, // F
        {14,17,16,16,19,17,14}, // G
        {17,17,17,31,17,17,17}, // H
        {14,4,4,4,4,4,14},      // I
        {1,1,1,1,17,17,14},     // J
        {17,18,20,24,20,18,17}, // K
        {16,16,16,16,16,16,31}, // L
        {17,27,21,21,17,17,17}, // M
        {17,25,21,19,17,17,17}, // N
        {14,17,17,17,17,17,14}, // O
        {30,17,17,30,16,16,16}, // P
        {14,17,17,17,21,18,13}, // Q
        {30,17,17,30,20,18,17}, // R
        {15,16,16,14,1,1,30},   // S
        {31,4,4,4,4,4,4},       // T
        {17,17,17,17,17,17,14}, // U
        {17,17,17,17,17,10,4},  // V
        {17,17,17,21,21,21,10}, // W
        {17,17,10,4,10,17,17},  // X
        {17,17,10,4,4,4,4},     // Y
        {31,1,2,4,8,16,31},     // Z
    };

    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }
    if (ch >= '0' && ch <= '9') {
        return g_digits[ch - '0'];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return g_upper[ch - 'A'];
    }
    switch (ch) {
        case ' ': return g_space;
        case '-': return g_dash;
        case '.': return g_dot;
        case ':': return g_colon;
        case '/': return g_slash;
        case '?': return g_qmark;
        default: return g_box;
    }
}

static void basalt_ssd_draw_char(int x, int y, char ch, int color) {
    const uint8_t *rows = basalt_ssd_debug_glyph(ch);
    for (int ry = 0; ry < 7; ++ry) {
        uint8_t row = rows[ry];
        for (int rx = 0; rx < 5; ++rx) {
            if (row & (1u << (4 - rx))) {
                basalt_ssd_put_pixel(x + rx, y + ry, color);
            }
        }
    }
}

static mp_obj_t basalt_ssd_ready(void) {
    return mp_obj_new_bool(basalt_ssd_init() == ESP_OK);
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ssd_ready_obj, basalt_ssd_ready);

static mp_obj_t basalt_ssd_width_obj(void) {
    return mp_obj_new_int(basalt_ssd_width());
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ssd_width_fun_obj, basalt_ssd_width_obj);

static mp_obj_t basalt_ssd_height_obj(void) {
    return mp_obj_new_int(basalt_ssd_height());
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ssd_height_fun_obj, basalt_ssd_height_obj);

static mp_obj_t basalt_ssd_show(void) {
    esp_err_t err = basalt_ssd_init();
    if (err != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("ssd1306 init failed (%d)"), (int)err);
    }
    const int w = basalt_ssd_width();
    const int pages = basalt_ssd_height() / 8;
    for (int p = 0; p < pages; ++p) {
        const uint8_t cmds[] = { (uint8_t)(0xB0 + p), 0x00, 0x10 };
        err = basalt_ssd_write_cmds(cmds, sizeof(cmds));
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("ssd1306 cmd failed (%d)"), (int)err);
        }
        err = basalt_ssd_write_data(&s_ssd_fb[p * w], (size_t)w);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("ssd1306 data failed (%d)"), (int)err);
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ssd_show_obj, basalt_ssd_show);

static mp_obj_t basalt_ssd_clear(size_t n_args, const mp_obj_t *args) {
    int color = 0;
    if (n_args >= 1) color = mp_obj_is_true(args[0]) ? 1 : 0;
    memset(s_ssd_fb, color ? 0xFF : 0x00, (size_t)basalt_ssd_fb_bytes());
    s_ssd_text_x = 0;
    s_ssd_text_y = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_clear_obj, 0, 1, basalt_ssd_clear);

static mp_obj_t basalt_ssd_color(mp_obj_t color_obj) {
    s_ssd_color = mp_obj_get_int(color_obj) ? 1 : 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_ssd_color_obj, basalt_ssd_color);

static mp_obj_t basalt_ssd_pixel(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int c = (n_args >= 3) ? (mp_obj_get_int(args[2]) ? 1 : 0) : s_ssd_color;
    basalt_ssd_put_pixel(x, y, c);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_pixel_obj, 2, 3, basalt_ssd_pixel);

static mp_obj_t basalt_ssd_line(size_t n_args, const mp_obj_t *args) {
    int x0 = mp_obj_get_int(args[0]);
    int y0 = mp_obj_get_int(args[1]);
    int x1 = mp_obj_get_int(args[2]);
    int y1 = mp_obj_get_int(args[3]);
    int c = (n_args >= 5) ? (mp_obj_get_int(args[4]) ? 1 : 0) : s_ssd_color;

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        basalt_ssd_put_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_line_obj, 4, 5, basalt_ssd_line);

static mp_obj_t basalt_ssd_rect(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    int c = (n_args >= 5) ? (mp_obj_get_int(args[4]) ? 1 : 0) : s_ssd_color;
    bool fill = (n_args >= 6) ? mp_obj_is_true(args[5]) : false;
    if (w <= 0 || h <= 0) return mp_const_none;

    if (fill) {
        for (int yy = 0; yy < h; ++yy) {
            basalt_ssd_hline(x, y + yy, w, c);
        }
    } else {
        basalt_ssd_hline(x, y, w, c);
        basalt_ssd_hline(x, y + h - 1, w, c);
        basalt_ssd_vline(x, y, h, c);
        basalt_ssd_vline(x + w - 1, y, h, c);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_rect_obj, 4, 6, basalt_ssd_rect);

static mp_obj_t basalt_ssd_circle(size_t n_args, const mp_obj_t *args) {
    int cx = mp_obj_get_int(args[0]);
    int cy = mp_obj_get_int(args[1]);
    int r = mp_obj_get_int(args[2]);
    int c = (n_args >= 4) ? (mp_obj_get_int(args[3]) ? 1 : 0) : s_ssd_color;
    bool fill = (n_args >= 5) ? mp_obj_is_true(args[4]) : false;
    if (r < 0) return mp_const_none;

    int x = r;
    int y = 0;
    int err = 1 - x;
    while (x >= y) {
        if (fill) {
            basalt_ssd_hline(cx - x, cy + y, 2 * x + 1, c);
            basalt_ssd_hline(cx - x, cy - y, 2 * x + 1, c);
            basalt_ssd_hline(cx - y, cy + x, 2 * y + 1, c);
            basalt_ssd_hline(cx - y, cy - x, 2 * y + 1, c);
        } else {
            basalt_ssd_put_pixel(cx + x, cy + y, c);
            basalt_ssd_put_pixel(cx + y, cy + x, c);
            basalt_ssd_put_pixel(cx - y, cy + x, c);
            basalt_ssd_put_pixel(cx - x, cy + y, c);
            basalt_ssd_put_pixel(cx - x, cy - y, c);
            basalt_ssd_put_pixel(cx - y, cy - x, c);
            basalt_ssd_put_pixel(cx + y, cy - x, c);
            basalt_ssd_put_pixel(cx + x, cy - y, c);
        }
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x + 1);
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_circle_obj, 3, 5, basalt_ssd_circle);

static mp_obj_t basalt_ssd_ellipse(size_t n_args, const mp_obj_t *args) {
    int cx = mp_obj_get_int(args[0]);
    int cy = mp_obj_get_int(args[1]);
    int rx = mp_obj_get_int(args[2]);
    int ry = mp_obj_get_int(args[3]);
    int c = (n_args >= 5) ? (mp_obj_get_int(args[4]) ? 1 : 0) : s_ssd_color;
    bool fill = (n_args >= 6) ? mp_obj_is_true(args[5]) : false;
    if (rx < 0 || ry < 0) return mp_const_none;

    long rx2 = (long)rx * rx;
    long ry2 = (long)ry * ry;
    long tworx2 = 2 * rx2;
    long twory2 = 2 * ry2;
    long x = 0;
    long y = ry;
    long px = 0;
    long py = tworx2 * y;
    long p = ry2 - (rx2 * ry) + (rx2 / 4);

    while (px < py) {
        if (fill) {
            basalt_ssd_hline(cx - (int)x, cy + (int)y, (int)(2 * x + 1), c);
            basalt_ssd_hline(cx - (int)x, cy - (int)y, (int)(2 * x + 1), c);
        } else {
            basalt_ssd_put_pixel(cx + (int)x, cy + (int)y, c);
            basalt_ssd_put_pixel(cx - (int)x, cy + (int)y, c);
            basalt_ssd_put_pixel(cx + (int)x, cy - (int)y, c);
            basalt_ssd_put_pixel(cx - (int)x, cy - (int)y, c);
        }
        x++;
        px += twory2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= tworx2;
            p += ry2 + px - py;
        }
    }

    p = ry2 * (x * x + x) + rx2 * (y - 1) * (y - 1) - rx2 * ry2 + (rx2 / 4);
    while (y >= 0) {
        if (fill) {
            basalt_ssd_hline(cx - (int)x, cy + (int)y, (int)(2 * x + 1), c);
            basalt_ssd_hline(cx - (int)x, cy - (int)y, (int)(2 * x + 1), c);
        } else {
            basalt_ssd_put_pixel(cx + (int)x, cy + (int)y, c);
            basalt_ssd_put_pixel(cx - (int)x, cy + (int)y, c);
            basalt_ssd_put_pixel(cx + (int)x, cy - (int)y, c);
            basalt_ssd_put_pixel(cx - (int)x, cy - (int)y, c);
        }
        y--;
        py -= tworx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twory2;
            p += rx2 - py + px;
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_ellipse_obj, 4, 6, basalt_ssd_ellipse);

static mp_obj_t basalt_ssd_text_at(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    size_t slen = 0;
    const char *s = mp_obj_str_get_data(args[2], &slen);
    int color = (n_args >= 4) ? (mp_obj_get_int(args[3]) ? 1 : 0) : s_ssd_color;
    const int w = basalt_ssd_width();
    const int h = basalt_ssd_height();

    int cx = x;
    int cy = y;
    for (size_t i = 0; i < slen; ++i) {
        char ch = s[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            cx = x;
            cy += 8;
            if (cy + 7 >= h) break;
            continue;
        }
        if (cx + 5 > w) {
            cx = x;
            cy += 8;
            if (cy + 7 >= h) break;
        }
        basalt_ssd_draw_char(cx, cy, ch, color);
        cx += 6;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_text_at_obj, 3, 4, basalt_ssd_text_at);

static mp_obj_t basalt_ssd_text(size_t n_args, const mp_obj_t *args) {
    int color = (n_args >= 2) ? (mp_obj_get_int(args[1]) ? 1 : 0) : s_ssd_color;
    mp_obj_t at_args[4] = {
        mp_obj_new_int(s_ssd_text_x),
        mp_obj_new_int(s_ssd_text_y),
        args[0],
        mp_obj_new_int(color),
    };
    basalt_ssd_text_at(4, at_args);

    size_t slen = 0;
    const char *s = mp_obj_str_get_data(args[0], &slen);
    int lines = 1;
    int last_line_chars = 0;
    for (size_t i = 0; i < slen; ++i) {
        if (s[i] == '\n') {
            lines++;
            last_line_chars = 0;
        } else if (s[i] != '\r') {
            last_line_chars++;
        }
    }
    if (lines > 1) {
        s_ssd_text_x = 0;
        s_ssd_text_y += lines * 8;
    } else {
        s_ssd_text_x += last_line_chars * 6;
    }
    if (s_ssd_text_x + 5 >= basalt_ssd_width()) {
        s_ssd_text_x = 0;
        s_ssd_text_y += 8;
    }
    if (s_ssd_text_y + 7 >= basalt_ssd_height()) {
        s_ssd_text_x = 0;
        s_ssd_text_y = 0;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ssd_text_obj, 1, 2, basalt_ssd_text);
#endif // BASALT_ENABLE_DISPLAY_SSD1306

void basalt_module_init(void) {
    qstr q_basalt = qstr_from_str("basalt");
    mp_obj_module_t *mod = mp_obj_new_module(q_basalt);

    mp_obj_module_t *gpio_mod = mp_obj_new_module(qstr_from_str("basalt_gpio"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("mode")),
        MP_OBJ_FROM_PTR(&basalt_gpio_mode_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("set")),
        MP_OBJ_FROM_PTR(&basalt_gpio_set_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("get")),
        MP_OBJ_FROM_PTR(&basalt_gpio_get_obj));

    mp_obj_module_t *timer_mod = mp_obj_new_module(qstr_from_str("basalt_timer"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(timer_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("sleep_ms")),
        MP_OBJ_FROM_PTR(&basalt_timer_sleep_ms_obj));

    mp_obj_module_t *led_mod = mp_obj_new_module(qstr_from_str("basalt_led"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(led_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("set")),
        MP_OBJ_FROM_PTR(&basalt_led_set_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(led_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("off")),
        MP_OBJ_FROM_PTR(&basalt_led_off_obj));

#if BASALT_ENABLE_RTC
    mp_obj_module_t *rtc_mod = mp_obj_new_module(qstr_from_str("basalt_rtc"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(rtc_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("available")),
        MP_OBJ_FROM_PTR(&basalt_rtc_available_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(rtc_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("now")),
        MP_OBJ_FROM_PTR(&basalt_rtc_now_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(rtc_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("address")),
        MP_OBJ_FROM_PTR(&basalt_rtc_address_fun_obj));
#endif
#if BASALT_ENABLE_DISPLAY_SSD1306
    mp_obj_module_t *ssd_mod = mp_obj_new_module(qstr_from_str("basalt_ssd1306"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ready")),
        MP_OBJ_FROM_PTR(&basalt_ssd_ready_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("width")),
        MP_OBJ_FROM_PTR(&basalt_ssd_width_fun_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("height")),
        MP_OBJ_FROM_PTR(&basalt_ssd_height_fun_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("show")),
        MP_OBJ_FROM_PTR(&basalt_ssd_show_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("clear")),
        MP_OBJ_FROM_PTR(&basalt_ssd_clear_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("color")),
        MP_OBJ_FROM_PTR(&basalt_ssd_color_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("pixel")),
        MP_OBJ_FROM_PTR(&basalt_ssd_pixel_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("line")),
        MP_OBJ_FROM_PTR(&basalt_ssd_line_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("rect")),
        MP_OBJ_FROM_PTR(&basalt_ssd_rect_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("circle")),
        MP_OBJ_FROM_PTR(&basalt_ssd_circle_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ellipse")),
        MP_OBJ_FROM_PTR(&basalt_ssd_ellipse_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("text_at")),
        MP_OBJ_FROM_PTR(&basalt_ssd_text_at_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ssd_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("text")),
        MP_OBJ_FROM_PTR(&basalt_ssd_text_obj));
#endif

    mp_obj_module_t *ui_mod = mp_obj_new_module(qstr_from_str("basalt_ui"));
    basalt_ui_init(ui_mod);

    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("gpio")),
        MP_OBJ_FROM_PTR(gpio_mod));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("timer")),
        MP_OBJ_FROM_PTR(timer_mod));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("led")),
        MP_OBJ_FROM_PTR(led_mod));
#if BASALT_ENABLE_RTC
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("rtc")),
        MP_OBJ_FROM_PTR(rtc_mod));
#endif
#if BASALT_ENABLE_DISPLAY_SSD1306
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ssd1306")),
        MP_OBJ_FROM_PTR(ssd_mod));
#endif
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ui")),
        MP_OBJ_FROM_PTR(ui_mod));

    mp_store_global(q_basalt, MP_OBJ_FROM_PTR(mod));
}
