#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

F="docs/ESP32_FIRST_SUCCESS_10_MIN.md"

require() {
  local pattern="$1"
  local msg="$2"
  if ! rg -q -- "$pattern" "$F"; then
    echo "FAIL: $msg"
    exit 1
  fi
}

require "# ESP32 First Success in 10 Minutes" "missing quickstart title"
require "--board esp32-c3-supermini" "missing reference board command"
require "Expected output snippet \(build\)" "missing build output expectation section"
require "Project build complete\." "missing concrete build output snippet"
require "Expected output snippet \(boot \+ shell\)" "missing boot/shell expectation section"
require "Basalt OS booted\. Type 'help'\." "missing shell boot expected line"
require "basalt> help -commands" "missing explicit help command expectation"
require "help run" "missing follow-up shell verification command"

echo "PASS: ESP32 first-success quickstart smoke checks passed"
