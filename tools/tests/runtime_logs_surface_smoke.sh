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

grepq "runtime_dispatch_is_ready_for" main/runtime_dispatch.h
grepq "runtime_dispatch_ready_detail_for" main/runtime_dispatch.h
grepq "runtime.ready.python" main/app_main.c
grepq "runtime.ready.lua" main/app_main.c
grepq "runtime.ready_detail.python" main/app_main.c
grepq "runtime.ready_detail.lua" main/app_main.c
grepq "runtime.guardrail_hint" main/app_main.c

echo "PASS: runtime logs surface smoke checks"
