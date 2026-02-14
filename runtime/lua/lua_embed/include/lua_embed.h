#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void lua_embed_init(void);
void lua_embed_deinit(void);
bool lua_embed_is_ready(void);

#ifdef __cplusplus
}
#endif
