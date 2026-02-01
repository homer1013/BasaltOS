#include "tft_console.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_log.h"
#include "esp_err.h"

// Board-generated pin assignments and feature gates
#include "basalt_config.h"

// -----------------------------------------------------------------------------
// Display selection
// -----------------------------------------------------------------------------
// BasaltOS currently supports two common TFT families:
//   - ST7796 (e.g. CYD_3248S035R / ESP32-3248S035R) 320x480
//   - ST7789V2 (e.g. M5StickC Plus2) 135x240 (panel is a window into a larger
//     internal GRAM, so we use offsets)
//
// Long-term: move these into a generic panel driver layer and select via boards
// JSON. For now we select based on the board macro.

#if defined(BASALT_BOARD_M5STICKC_PLUS2)
    #define BASALT_TFT_DRIVER_ST7789V2 1
    #define BASALT_TFT_WIDTH   135
    #define BASALT_TFT_HEIGHT  240
    // Common offsets for 135x240 ST7789 panels (M5StickC family)
    #define BASALT_TFT_X_OFFSET 52
    #define BASALT_TFT_Y_OFFSET 40
#elif defined(BASALT_BOARD_CYD_3248S035R)
    #define BASALT_TFT_DRIVER_ST7796 1
    #define BASALT_TFT_WIDTH   320
    #define BASALT_TFT_HEIGHT  480
    #define BASALT_TFT_X_OFFSET 0
    #define BASALT_TFT_Y_OFFSET 0
#else
    // Safe default (keeps existing behaviour)
    #define BASALT_TFT_DRIVER_ST7796 1
    #define BASALT_TFT_WIDTH   320
    #define BASALT_TFT_HEIGHT  480
    #define BASALT_TFT_X_OFFSET 0
    #define BASALT_TFT_Y_OFFSET 0
#endif

// --- Pins (generated from boards JSON) ---
#define BASALT_TFT_ENABLE 1

#define BASALT_TFT_HOST   SPI3_HOST
#define BASALT_TFT_MOSI   BASALT_PIN_TFT_MOSI
#ifdef BASALT_PIN_TFT_MISO
    #define BASALT_TFT_MISO BASALT_PIN_TFT_MISO
#else
    #define BASALT_TFT_MISO -1
#endif
#define BASALT_TFT_SCLK   BASALT_PIN_TFT_SCLK
#define BASALT_TFT_CS     BASALT_PIN_TFT_CS
#define BASALT_TFT_DC     BASALT_PIN_TFT_DC
#ifdef BASALT_PIN_TFT_RST
    #define BASALT_TFT_RST BASALT_PIN_TFT_RST
#else
    #define BASALT_TFT_RST -1
#endif
#ifdef BASALT_PIN_TFT_BL
    #define BASALT_TFT_BCKL BASALT_PIN_TFT_BL
#else
    #define BASALT_TFT_BCKL -1
#endif

#define BASALT_TFT_CLK_HZ 10000000
#define BASALT_TFT_SPI_MODE 0
#define BASALT_TFT_TEST_PATTERN 0
#if defined(BASALT_TFT_DRIVER_ST7789V2)
    // ST7789V2 on M5StickC Plus2 needs no MX to avoid left-right mirroring.
    #define BASALT_TFT_MADCTL 0x08
#else
    #define BASALT_TFT_MADCTL 0x48
#endif
#define BASALT_TFT_PIXFMT 0x55 // 16-bit (RGB565) for ST7796

#define FONT_W 6
#define FONT_H 8
#define MAX_COLS (BASALT_TFT_WIDTH / FONT_W)
#define MAX_ROWS (BASALT_TFT_HEIGHT / FONT_H)

static const char *TAG = "tft";

static spi_device_handle_t s_spi = NULL;
static bool s_ready = false;
static char s_screen[MAX_ROWS][MAX_COLS];
static uint16_t s_color[MAX_ROWS][MAX_COLS];
static uint16_t s_linebuf[BASALT_TFT_WIDTH * FONT_H];
static uint16_t s_fg = 0xFFFF;
static uint16_t s_bg = 0x0000;
static int s_row = 0;
static int s_col = 0;
static SemaphoreHandle_t s_tft_lock = NULL;

// 5x7 ASCII font, characters 32..127 (96 glyphs). Public domain style.
static const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x7F,0x20,0x18,0x20,0x7F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x03,0x04,0x78,0x04,0x03}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x03,0x05,0x00,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x10,0x08,0x08,0x10,0x08}, // '~'
    {0x00,0x00,0x00,0x00,0x00}  // DEL
};

static void tft_write_cmd(uint8_t cmd) {
    gpio_set_level(BASALT_TFT_DC, 0);
    spi_transaction_t t = {0};
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi, &t);
}

static void tft_write_data(const uint8_t *data, int len) {
    if (len <= 0) return;
    gpio_set_level(BASALT_TFT_DC, 1);
    spi_transaction_t t = {0};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi, &t);
}

// Convenience for single-byte data writes
static inline void tft_write_u8(uint8_t v) {
    tft_write_data(&v, 1);
}

static void tft_reset(void) {
    if (BASALT_TFT_RST < 0) return;
    gpio_set_level(BASALT_TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(BASALT_TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void tft_st7796_init(void) {
    // ST7796S init sequence (based on common TFT_eSPI configs)
    tft_write_cmd(0xF0);
    uint8_t f0_1[] = {0xC3};
    tft_write_data(f0_1, sizeof(f0_1));
    tft_write_cmd(0xF0);
    uint8_t f0_2[] = {0x96};
    tft_write_data(f0_2, sizeof(f0_2));

    tft_write_cmd(0x36); // MADCTL
    uint8_t mad = BASALT_TFT_MADCTL;
    tft_write_data(&mad, 1);

    tft_write_cmd(0x3A); // COLMOD
    uint8_t pf = BASALT_TFT_PIXFMT;
    tft_write_data(&pf, 1);

    tft_write_cmd(0xB4);
    uint8_t inv[] = {0x01};
    tft_write_data(inv, sizeof(inv));

    tft_write_cmd(0xB7);
    uint8_t gate[] = {0xC6};
    tft_write_data(gate, sizeof(gate));

    tft_write_cmd(0xC0);
    uint8_t pwr1[] = {0x80, 0x45};
    tft_write_data(pwr1, sizeof(pwr1));

    tft_write_cmd(0xC1);
    uint8_t pwr2[] = {0x13};
    tft_write_data(pwr2, sizeof(pwr2));

    tft_write_cmd(0xC2);
    uint8_t pwr3[] = {0xA7};
    tft_write_data(pwr3, sizeof(pwr3));

    tft_write_cmd(0xC5);
    uint8_t vcom[] = {0x0A};
    tft_write_data(vcom, sizeof(vcom));

    tft_write_cmd(0xE8);
    uint8_t e8[] = {0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33};
    tft_write_data(e8, sizeof(e8));

    tft_write_cmd(0xE0);
    uint8_t pgamma[] = {0xF0,0x06,0x0B,0x0A,0x09,0x26,0x29,0x33,0x41,0x18,0x16,0x15,0x29,0x2D};
    tft_write_data(pgamma, sizeof(pgamma));

    tft_write_cmd(0xE1);
    uint8_t ngamma[] = {0xF0,0x04,0x0C,0x0A,0x08,0x25,0x26,0x32,0x3C,0x18,0x16,0x15,0x29,0x2D};
    tft_write_data(ngamma, sizeof(ngamma));

    tft_write_cmd(0xF0);
    uint8_t f0_3[] = {0x3C};
    tft_write_data(f0_3, sizeof(f0_3));
    tft_write_cmd(0xF0);
    uint8_t f0_4[] = {0x69};
    tft_write_data(f0_4, sizeof(f0_4));
}

// Minimal init for ST7789V2 (as used on M5StickC Plus2 and similar 135x240 panels).
// This is intentionally conservative; we can tune gamma/power settings later.
static void tft_st7789v2_init(void) {
    // Porch setting
    tft_write_cmd(0xB2);
    uint8_t b2[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    tft_write_data(b2, sizeof(b2));

    // Gate control
    tft_write_cmd(0xB7);
    uint8_t b7 = 0x35;
    tft_write_data(&b7, 1);

    // VCOM setting
    tft_write_cmd(0xBB);
    uint8_t bb = 0x19;
    tft_write_data(&bb, 1);

    // LCM control
    tft_write_cmd(0xC0);
    uint8_t c0 = 0x2C;
    tft_write_data(&c0, 1);

    // VDV and VRH command enable
    tft_write_cmd(0xC2);
    uint8_t c2 = 0x01;
    tft_write_data(&c2, 1);

    // VRH set
    tft_write_cmd(0xC3);
    uint8_t c3 = 0x12;
    tft_write_data(&c3, 1);

    // VDV set
    tft_write_cmd(0xC4);
    uint8_t c4 = 0x20;
    tft_write_data(&c4, 1);

    // Frame rate control
    tft_write_cmd(0xC6);
    uint8_t c6 = 0x0F;
    tft_write_data(&c6, 1);

    // Power control 1
    tft_write_cmd(0xD0);
    uint8_t d0[] = {0xA4, 0xA1};
    tft_write_data(d0, sizeof(d0));

    // Positive gamma
    tft_write_cmd(0xE0);
    uint8_t e0[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54,
                    0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    tft_write_data(e0, sizeof(e0));

    // Negative gamma
    tft_write_cmd(0xE1);
    uint8_t e1[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44,
                    0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    tft_write_data(e1, sizeof(e1));

    // Inversion on (common for these small IPS panels)
    tft_write_cmd(0x21);
}

static void tft_set_addr_window(int x0, int y0, int x1, int y1) {
    // Some panels (notably 135x240 ST7789 variants) use an internal GRAM
    // that is larger than the visible area; Basalt uses per-board offsets.
    x0 += BASALT_TFT_X_OFFSET;
    x1 += BASALT_TFT_X_OFFSET;
    y0 += BASALT_TFT_Y_OFFSET;
    y1 += BASALT_TFT_Y_OFFSET;
    uint8_t data[4];
    tft_write_cmd(0x2A);
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    tft_write_data(data, 4);

    tft_write_cmd(0x2B);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    tft_write_data(data, 4);

    tft_write_cmd(0x2C);
}

static void tft_push_colors(const uint16_t *data, int len) {
    gpio_set_level(BASALT_TFT_DC, 1);
    spi_transaction_t t = {0};
    t.length = len * 16;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi, &t);
}

static void tft_draw_line(int row) {
    if (row < 0 || row >= MAX_ROWS) return;
    int y0 = row * FONT_H;
    int y1 = y0 + FONT_H - 1;

    for (int x = 0; x < BASALT_TFT_WIDTH * FONT_H; x++) {
        s_linebuf[x] = s_bg;
    }

    for (int col = 0; col < MAX_COLS; col++) {
        char ch = s_screen[row][col];
        if (ch < 32 || ch > 127) ch = '?';
        const uint8_t *glyph = font5x7[ch - 32];
        uint16_t fg = s_color[row][col];
        int x0 = col * FONT_W;
        for (int gx = 0; gx < 5; gx++) {
            uint8_t bits = glyph[gx];
            for (int gy = 0; gy < 7; gy++) {
                if (bits & (1 << gy)) {
                    int px = x0 + gx;
                    int py = gy + 1; // small top padding
                    int idx = py * BASALT_TFT_WIDTH + px;
                    if (idx >= 0 && idx < BASALT_TFT_WIDTH * FONT_H) {
                        s_linebuf[idx] = fg;
                    }
                }
            }
        }
    }

    tft_set_addr_window(0, y0, BASALT_TFT_WIDTH - 1, y1);
    tft_push_colors(s_linebuf, BASALT_TFT_WIDTH * FONT_H);
}

static void tft_clear_screen(void) {
    // Clear text buffer
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            s_screen[r][c] = ' ';
            s_color[r][c] = s_fg;
        }
    }
    // Fill display with background color (black)
    for (int x = 0; x < BASALT_TFT_WIDTH * FONT_H; x++) {
        s_linebuf[x] = s_bg;
    }
    for (int y = 0; y < BASALT_TFT_HEIGHT; y += FONT_H) {
        int y1 = y + FONT_H - 1;
        if (y1 >= BASALT_TFT_HEIGHT) y1 = BASALT_TFT_HEIGHT - 1;
        tft_set_addr_window(0, y, BASALT_TFT_WIDTH - 1, y1);
        tft_push_colors(s_linebuf, BASALT_TFT_WIDTH * (y1 - y + 1));
    }
}

static void tft_scroll(void) {
    for (int r = 1; r < MAX_ROWS; r++) {
        memcpy(s_screen[r - 1], s_screen[r], MAX_COLS);
        memcpy(s_color[r - 1], s_color[r], MAX_COLS * sizeof(uint16_t));
    }
    for (int c = 0; c < MAX_COLS; c++) {
        s_screen[MAX_ROWS - 1][c] = ' ';
        s_color[MAX_ROWS - 1][c] = s_fg;
    }
    for (int r = 0; r < MAX_ROWS; r++) {
        tft_draw_line(r);
    }
}

void tft_console_write(const char *text) {
    if (!s_ready || !text) return;
    if (s_tft_lock) {
        xSemaphoreTake(s_tft_lock, portMAX_DELAY);
    }
    bool dirty = false;

    for (const char *p = text; *p; p++) {
        char ch = *p;
        if (ch == '\n') {
            tft_draw_line(s_row);
            s_row++;
            s_col = 0;
            dirty = false;
            if (s_row >= MAX_ROWS) {
                s_row = MAX_ROWS - 1;
                tft_scroll();
            }
            continue;
        }

        if (ch == '\r') continue;

        if (s_row < 0) s_row = 0;
        if (s_row >= MAX_ROWS) s_row = MAX_ROWS - 1;
        if (s_col < 0) s_col = 0;
        if (s_col >= MAX_COLS) {
            tft_draw_line(s_row);
            s_row++;
            s_col = 0;
            dirty = false;
            if (s_row >= MAX_ROWS) {
                s_row = MAX_ROWS - 1;
                tft_scroll();
            }
        }

        s_screen[s_row][s_col] = ch;
        s_color[s_row][s_col] = s_fg;
        s_col++;
        dirty = true;

        if (s_col >= MAX_COLS) {
            tft_draw_line(s_row);
            s_row++;
            s_col = 0;
            dirty = false;
            if (s_row >= MAX_ROWS) {
                s_row = MAX_ROWS - 1;
                tft_scroll();
            }
        }
    }

    if (dirty) {
        tft_draw_line(s_row);
    }
    if (s_tft_lock) {
        xSemaphoreGive(s_tft_lock);
    }
}

bool tft_console_is_ready(void) {
    return s_ready;
}

void tft_console_set_color(uint16_t fg) {
    s_fg = fg;
}

bool tft_console_init(void) {
#if BASALT_TFT_ENABLE
    if (!s_tft_lock) {
        s_tft_lock = xSemaphoreCreateMutex();
    }
    uint64_t pin_mask = (1ULL << BASALT_TFT_DC);
#if BASALT_TFT_RST >= 0
    pin_mask |= (1ULL << BASALT_TFT_RST);
#endif
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

#if BASALT_TFT_BCKL >= 0
    gpio_config_t bl_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    bl_conf.pin_bit_mask = (1ULL << BASALT_TFT_BCKL);
    gpio_config(&bl_conf);
    gpio_set_level(BASALT_TFT_BCKL, 1);
#endif

    spi_bus_config_t buscfg = {
        .mosi_io_num = BASALT_TFT_MOSI,
        .miso_io_num = BASALT_TFT_MISO,
        .sclk_io_num = BASALT_TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BASALT_TFT_WIDTH * FONT_H * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(BASALT_TFT_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = BASALT_TFT_CLK_HZ,
        .mode = BASALT_TFT_SPI_MODE,
        .spics_io_num = BASALT_TFT_CS,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .pre_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(BASALT_TFT_HOST, &devcfg, &s_spi));

    tft_reset();

    tft_write_cmd(0x01); // software reset
    vTaskDelay(pdMS_TO_TICKS(120));
    // MADCTL default: BGR, no mirroring (panel-specific rotations can override later).
    tft_write_cmd(0x36);
    tft_write_u8(0x08);
    // Memory access control (MADCTL): fix left-right mirroring on some ST7789 panels.
    // 0x08 = BGR color order, no MX/MY/MV (rotation handled by offsets in set_window).
    tft_write_cmd(0x36);
    tft_write_u8(0x08);
    tft_write_cmd(0x11); // sleep out
    vTaskDelay(pdMS_TO_TICKS(120));

    // Panel-specific init sequence
#if BASALT_TFT_DRIVER_ST7796
    tft_st7796_init();
#elif BASALT_TFT_DRIVER_ST7789V2
    tft_st7789v2_init();
#else
    tft_st7796_init();
#endif

    tft_write_cmd(0x3A); // pixel format
    uint8_t pf = BASALT_TFT_PIXFMT;
    tft_write_data(&pf, 1);

    tft_write_cmd(0x36); // memory access control
    uint8_t mad = BASALT_TFT_MADCTL;
    tft_write_data(&mad, 1);

    tft_write_cmd(0x29); // display on
    vTaskDelay(pdMS_TO_TICKS(20));

#if BASALT_TFT_TEST_PATTERN
    tft_fill_color(0xF800); // red
    vTaskDelay(pdMS_TO_TICKS(200));
    tft_fill_color(0x07E0); // green
    vTaskDelay(pdMS_TO_TICKS(200));
    tft_fill_color(0x001F); // blue
    vTaskDelay(pdMS_TO_TICKS(200));
#endif

    tft_clear_screen();
    s_ready = true;
    ESP_LOGI(TAG, "TFT console ready (%dx%d)", BASALT_TFT_WIDTH, BASALT_TFT_HEIGHT);
    return true;
#else
    return false;
#endif
}
