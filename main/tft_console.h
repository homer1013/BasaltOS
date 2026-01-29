#pragma once

#include <stdbool.h>
#include <stdint.h>

bool tft_console_init(void);
bool tft_console_is_ready(void);
void tft_console_write(const char *text);
void tft_console_set_color(uint16_t fg);
