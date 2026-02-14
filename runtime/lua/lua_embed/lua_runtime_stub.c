#include "lua_embed.h"

static bool s_lua_ready = false;

void lua_embed_init(void) {
    s_lua_ready = false;
}

void lua_embed_deinit(void) {
    s_lua_ready = false;
}

bool lua_embed_is_ready(void) {
    return s_lua_ready;
}
