#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

grepq() {
  local pattern="$1"
  local file="$2"
  if command -v rg >/dev/null 2>&1; then
    rg -q "$pattern" "$file"
  else
    grep -Fq "$pattern" "$file"
  fi
}

test -f runtime/lua/lua_embed/include/basalt_lua_bindings.h
test -f runtime/lua/lua_embed/basalt_lua_bindings.c

grepq "system_log" runtime/lua/lua_embed/include/basalt_lua_bindings.h
grepq "gpio_mode" runtime/lua/lua_embed/include/basalt_lua_bindings.h
grepq "timer_sleep_ms" runtime/lua/lua_embed/include/basalt_lua_bindings.h
grepq "fs_write_text" runtime/lua/lua_embed/include/basalt_lua_bindings.h
grepq "fs_read_text" runtime/lua/lua_embed/include/basalt_lua_bindings.h

grepq "hal_gpio_set_mode" runtime/lua/lua_embed/basalt_lua_bindings.c
grepq "hal_gpio_write" runtime/lua/lua_embed/basalt_lua_bindings.c
grepq "hal_gpio_read" runtime/lua/lua_embed/basalt_lua_bindings.c
grepq "vTaskDelay" runtime/lua/lua_embed/basalt_lua_bindings.c
grepq "fopen" runtime/lua/lua_embed/basalt_lua_bindings.c
grepq "fread" runtime/lua/lua_embed/basalt_lua_bindings.c

grepq "lua_embed_init" main/lua_runtime.c
grepq "lua runtime is not ready" main/lua_runtime.c

echo "PASS: lua bindings contract smoke checks"
