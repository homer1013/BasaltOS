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
    grep -q "$pattern" "$file"
  fi
}

grepq "BASALT_LUA_MAX_SCRIPT_BYTES" main/lua_runtime.c
grepq "BASALT_LUA_MIN_FREE_HEAP_BYTES" main/lua_runtime.c
grepq "heap_caps_get_free_size" main/lua_runtime.c
grepq "script too large" main/lua_runtime.c
grepq "free heap too low" main/lua_runtime.c
grepq "guardrail-blocked" main/lua_runtime.c
grepq "lua_runtime_check_guardrails\\(path" main/lua_runtime.c

echo "PASS: lua runtime guardrails smoke checks"
