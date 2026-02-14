#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

rg -q "runtime_dispatch_is_ready_for" main/runtime_dispatch.h
rg -q "runtime_dispatch_ready_detail_for" main/runtime_dispatch.h
rg -q "runtime.ready.python" main/app_main.c
rg -q "runtime.ready.lua" main/app_main.c
rg -q "runtime.ready_detail.python" main/app_main.c
rg -q "runtime.ready_detail.lua" main/app_main.c
rg -q "runtime.guardrail_hint" main/app_main.c

echo "PASS: runtime logs surface smoke checks"
