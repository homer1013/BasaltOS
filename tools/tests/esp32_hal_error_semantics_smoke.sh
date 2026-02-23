#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

esp32_sources = sorted(Path("basalt_hal/ports/esp32").glob("hal_*.c"))
esp32c3_sources = sorted(Path("basalt_hal/ports/esp32c3").glob("hal_*.c"))
esp32c6_sources = sorted(Path("basalt_hal/ports/esp32c6").glob("hal_*.c"))
esp32s3_sources = sorted(Path("basalt_hal/ports/esp32s3").glob("hal_*.c"))
all_sources = esp32_sources + esp32c3_sources + esp32c6_sources + esp32s3_sources
if not all_sources:
    raise SystemExit("FAIL: no ESP32-family HAL sources found")

missing_helper = []
local_mappers = []
stale_ealready = []

for src in all_sources:
    text = src.read_text(encoding="utf-8")
    uses_esp_errors = ("esp_err_t" in text) or ("ESP_ERR_" in text)
    if uses_esp_errors:
        if '#include "hal_errno.h"' not in text:
            missing_helper.append(str(src))
        if "static inline int esp_err_to_errno(" in text:
            local_mappers.append(str(src))
        if "ESP_ERR_INVALID_STATE: return -EALREADY" in text:
            stale_ealready.append(str(src))
        if "return -EALREADY;" in text:
            stale_ealready.append(str(src))

if missing_helper:
    raise SystemExit(f"FAIL: HAL files missing shared errno helper include: {missing_helper}")
if local_mappers:
    raise SystemExit(f"FAIL: HAL files still define local esp_err_to_errno mapper: {local_mappers}")
if stale_ealready:
    stale = sorted(set(stale_ealready))
    raise SystemExit(f"FAIL: HAL files still return -EALREADY: {stale}")

print("PASS: ESP32-family HAL error semantics smoke checks")
PY
