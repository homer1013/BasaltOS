#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*system_log)(const char *msg);
    int (*gpio_mode)(int pin, int mode);
    int (*gpio_write)(int pin, int value);
    int (*gpio_read)(int pin, int *value);
    int (*timer_sleep_ms)(uint32_t ms);
} basalt_lua_bindings_api_t;

void basalt_lua_bindings_init(void);
bool basalt_lua_bindings_ready(void);
const basalt_lua_bindings_api_t *basalt_lua_bindings_get(void);
const char *basalt_lua_bindings_summary(void);

#ifdef __cplusplus
}
#endif
