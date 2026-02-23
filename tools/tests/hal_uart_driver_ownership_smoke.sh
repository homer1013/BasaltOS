#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    path = Path(f"basalt_hal/ports/{port}/hal_uart.c")
    text = path.read_text(encoding="utf-8")
    required = [
        "bool driver_owner;",
        "iu->driver_owner = false;",
        "if (e == ESP_OK) {",
        "u->driver_owner = true;",
        "} else if (e == ESP_ERR_INVALID_STATE) {",
        "u->driver_owner = false;",
        "if (iu->driver_owner) {",
    ]
    for needle in required:
        if needle not in text:
            raise SystemExit(f"FAIL: missing '{needle}' in {path}")

print("PASS: HAL UART driver ownership smoke checks")
PY
