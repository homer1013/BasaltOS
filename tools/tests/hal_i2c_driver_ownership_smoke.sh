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
        "bool driver_owner;",
        "h->driver_owner = false;",
        "if (e2 == ESP_OK) {",
        "h->driver_owner = true;",
        "if (h->driver_owner) {",
        "h->driver_owner = false;",
    ]
    for needle in required:
        if needle not in text:
            raise SystemExit(f"FAIL: missing '{needle}' in {path}")

print("PASS: HAL I2C driver ownership smoke checks")
PY
