#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    path = Path(f"basalt_hal/ports/{port}/hal_i2c.c")
    text = path.read_text(encoding="utf-8")
    required = [
        "if (timeout_ms == 0) return 0;",
        "if (timeout_ms == UINT32_MAX) return portMAX_DELAY;",
        "return pdMS_TO_TICKS(timeout_ms);",
    ]
    for needle in required:
        if needle not in text:
            raise SystemExit(f"FAIL: missing timeout semantic '{needle}' in {path}")

print("PASS: HAL I2C timeout semantics smoke checks")
PY
