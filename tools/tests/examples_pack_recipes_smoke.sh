#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

F="docs/EXAMPLES_PACK_RECIPES.md"

require() {
  local pattern="$1"
  local msg="$2"
  if ! rg -q -- "$pattern" "$F"; then
    echo "FAIL: $msg"
    exit 1
  fi
}

require "# BasaltOS Examples Pack" "missing examples pack title"
require "## 1\) OLED Demo Recipe" "missing OLED recipe section"
require "display_ssd1306" "missing OLED driver reference"
require "## 2\) DC Motor Demo Recipe" "missing DC motor recipe section"
require "l298n test" "missing L298N command coverage"
require "## 3\) Sensor Demo Recipe" "missing sensor recipe section"
require "dht22 read 10 auto" "missing DHT read recipe command"
require "docs/SSD1306_HARDWARE_VALIDATION.md" "missing OLED deep-link"
require "docs/L298N_ULN2003_HARDWARE_VALIDATION.md" "missing motor deep-link"
require "docs/DHT22_HARDWARE_VALIDATION.md" "missing sensor deep-link"

echo "PASS: examples pack recipes smoke checks passed"
