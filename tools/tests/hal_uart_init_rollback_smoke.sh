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
        "static void uart_rollback_driver_if_owned",
        "uart_rollback_driver_if_owned(u);",
    ]
    for needle in required:
        if needle not in text:
            raise SystemExit(f"FAIL: missing UART init rollback guard '{needle}' in {path}")
    if text.count("uart_rollback_driver_if_owned(u);") < 2:
        raise SystemExit(f"FAIL: expected rollback on both pin/flow failures in {path}")

print("PASS: HAL UART init rollback smoke checks")
PY
