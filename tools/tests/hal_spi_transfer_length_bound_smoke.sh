#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    path = Path(f"basalt_hal/ports/{port}/hal_spi.c")
    text = path.read_text(encoding="utf-8")

    if "#include <limits.h>" not in text:
        raise SystemExit(f"FAIL: limits.h include missing in {path}")

    guard = "if (len > ((size_t)INT_MAX / 8U)) return -EMSGSIZE;"
    if guard not in text:
        raise SystemExit(f"FAIL: transfer length bound guard missing in {path}")

print("PASS: HAL SPI transfer length bound smoke checks")
PY
