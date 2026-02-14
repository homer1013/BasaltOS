#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

# shellcheck source=/dev/null
source tools/env.sh

bash tools/tests/app_runtime_manifest_smoke.sh
bash tools/tests/lua_bindings_contract_smoke.sh
bash tools/tests/lua_runtime_guardrails_smoke.sh
bash tools/tests/runtime_logs_surface_smoke.sh
bash tools/tests/lua_examples_parity_smoke.sh

idf.py -B build_c6_lua_ci_default set-target esp32c6
idf.py -B build_c6_lua_ci_default build
idf.py -B build_c6_lua_ci_lua -D BASALT_ENABLE_LUA_RUNTIME=ON set-target esp32c6
idf.py -B build_c6_lua_ci_lua -D BASALT_ENABLE_LUA_RUNTIME=ON build

echo "PASS: lua runtime CI smoke checks"
