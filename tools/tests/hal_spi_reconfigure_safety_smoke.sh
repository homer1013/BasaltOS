#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import re

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    path = Path(f"basalt_hal/ports/{port}/hal_spi.c")
    text = path.read_text(encoding="utf-8")
    funcs = re.findall(r"int\s+hal_spi_set_(?:freq|mode)\s*\([^)]*\)\s*\{(.*?)\n\}", text, flags=re.S)
    if len(funcs) != 2:
        raise SystemExit(f"FAIL: expected hal_spi_set_freq/mode in {path}")
    for body in funcs:
        if "spi_bus_remove_device(s->dev);" not in body:
            raise SystemExit(f"FAIL: remove-device call missing in {path}")
        if "s->dev = NULL;" not in body:
            raise SystemExit(f"FAIL: stale-device guard missing in {path}")
        if body.index("spi_bus_remove_device(s->dev);") > body.index("s->dev = NULL;"):
            raise SystemExit(f"FAIL: stale-device guard order invalid in {path}")

print("PASS: HAL SPI reconfigure safety smoke checks")
PY
