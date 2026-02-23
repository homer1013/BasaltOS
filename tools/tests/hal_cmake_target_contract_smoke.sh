#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import re

cmake = Path("basalt_hal/CMakeLists.txt").read_text(encoding="utf-8")
targets = re.findall(r'IDF_TARGET STREQUAL "([^"]+)"', cmake)
actual = set(targets)
expected = {"esp32", "esp32c3", "esp32c6", "esp32s3"}

if actual != expected:
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    raise SystemExit(
        f"FAIL: basalt_hal/CMakeLists.txt IDF target contract mismatch. "
        f"missing={missing} extra={extra}"
    )

for src in ("hal_spi.c", "hal_timer.c", "hal_i2s.c", "hal_rmt.c"):
    if src not in cmake:
        raise SystemExit(f"FAIL: basalt_hal/CMakeLists.txt missing required source '{src}'")

print("PASS: HAL CMake target contract smoke checks")
PY
