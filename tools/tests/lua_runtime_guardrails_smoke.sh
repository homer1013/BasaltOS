#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

rg -q "BASALT_LUA_MAX_SCRIPT_BYTES" main/lua_runtime.c
rg -q "BASALT_LUA_MIN_FREE_HEAP_BYTES" main/lua_runtime.c
rg -q "heap_caps_get_free_size" main/lua_runtime.c
rg -q "script too large" main/lua_runtime.c
rg -q "free heap too low" main/lua_runtime.c
rg -q "guardrail-blocked" main/lua_runtime.c
rg -q "lua_runtime_check_guardrails\\(path" main/lua_runtime.c

echo "PASS: lua runtime guardrails smoke checks"
