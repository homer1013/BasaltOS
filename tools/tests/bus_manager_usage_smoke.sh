#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

require_line() {
  local pattern="$1"
  local file="$2"
  if ! rg -q --fixed-strings "$pattern" "$file"; then
    echo "FAIL: expected '$pattern' in $file" >&2
    exit 1
  fi
}

reject_main_direct_calls() {
  local pat='spi_bus_initialize|i2c_param_config|i2c_driver_install'
  if rg -n "$pat" main --glob '!main/bus_manager.c' --glob '!main/bus_manager.h'; then
    echo "FAIL: direct bus init calls found outside bus_manager in main/" >&2
    exit 1
  fi
}

if [[ ! -f main/bus_manager.c || ! -f main/bus_manager.h ]]; then
  echo "FAIL: bus manager files missing" >&2
  exit 1
fi

require_line "#include \"bus_manager.h\"" "main/app_main.c"
require_line "#include \"bus_manager.h\"" "main/tft_console.c"
require_line "\"bus_manager.c\"" "main/CMakeLists.txt"
reject_main_direct_calls

echo "PASS: bus manager usage smoke checks"
