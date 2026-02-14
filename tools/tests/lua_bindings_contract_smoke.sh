#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

test -f runtime/lua/lua_embed/include/basalt_lua_bindings.h
test -f runtime/lua/lua_embed/basalt_lua_bindings.c

rg -q "system_log" runtime/lua/lua_embed/include/basalt_lua_bindings.h
rg -q "gpio_mode" runtime/lua/lua_embed/include/basalt_lua_bindings.h
rg -q "timer_sleep_ms" runtime/lua/lua_embed/include/basalt_lua_bindings.h

rg -q "hal_gpio_set_mode" runtime/lua/lua_embed/basalt_lua_bindings.c
rg -q "hal_gpio_write" runtime/lua/lua_embed/basalt_lua_bindings.c
rg -q "hal_gpio_read" runtime/lua/lua_embed/basalt_lua_bindings.c
rg -q "vTaskDelay" runtime/lua/lua_embed/basalt_lua_bindings.c

rg -q "lua_embed_init" main/lua_runtime.c
rg -q "lua runtime is not ready" main/lua_runtime.c

echo "PASS: lua bindings contract smoke checks"
