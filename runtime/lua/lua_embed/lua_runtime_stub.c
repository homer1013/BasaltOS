#include "lua_embed.h"
#include "basalt_lua_bindings.h"

static bool s_lua_ready = false;

void lua_embed_init(void) {
    basalt_lua_bindings_init();
    s_lua_ready = basalt_lua_bindings_ready();
}

void lua_embed_deinit(void) {
    s_lua_ready = false;
}

bool lua_embed_is_ready(void) {
    return s_lua_ready;
}
