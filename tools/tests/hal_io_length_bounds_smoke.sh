#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    i2c = Path(f"basalt_hal/ports/{port}/hal_i2c.c")
    uart = Path(f"basalt_hal/ports/{port}/hal_uart.c")

    i2c_text = i2c.read_text(encoding="utf-8")
    uart_text = uart.read_text(encoding="utf-8")

    if "#include <limits.h>" not in i2c_text:
        raise SystemExit(f"FAIL: limits.h include missing in {i2c}")
    if "#include <limits.h>" not in uart_text:
        raise SystemExit(f"FAIL: limits.h include missing in {uart}")

    for guard in [
        "if (len > (size_t)INT_MAX) return -EMSGSIZE;",
        "if (rlen > (size_t)INT_MAX) return -EMSGSIZE;",
    ]:
        if guard not in i2c_text:
            raise SystemExit(f"FAIL: missing I2C length guard '{guard}' in {i2c}")

    uart_guard_count = uart_text.count("if (len > (size_t)INT_MAX) return -EMSGSIZE;")
    if uart_guard_count < 2:
        raise SystemExit(f"FAIL: expected send/recv length guards in {uart}")

print("PASS: HAL I/O length bound smoke checks")
PY
