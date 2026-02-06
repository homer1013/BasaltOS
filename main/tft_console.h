#pragma once

#include <stdbool.h>
#include <stdint.h>

bool tft_console_init(void);
bool tft_console_is_ready(void);
void tft_console_write(const char *text);
void tft_console_set_color(uint16_t fg);
void tft_console_clear(void);
void tft_console_write_at(int x, int y, const char *text);
void tft_console_draw_pixel(int x, int y, uint16_t color);
void tft_console_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void tft_console_draw_rect(int x, int y, int w, int h, uint16_t color, bool fill);
void tft_console_draw_circle(int cx, int cy, int r, uint16_t color, bool fill);
void tft_console_draw_ellipse(int cx, int cy, int rx, int ry, uint16_t color, bool fill);
bool tft_console_touch_read(int *pressed, int *x, int *y, int *raw_x, int *raw_y);
