#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32c3", "esp32c6", "esp32s3"]
core_prims = ["spi", "adc", "pwm", "timer"]
esp32_prims = ["i2s", "rmt"]

required_headers = [
    Path("basalt_hal/include/hal/hal_spi.h"),
    Path("basalt_hal/include/hal/hal_adc.h"),
    Path("basalt_hal/include/hal/hal_pwm.h"),
    Path("basalt_hal/include/hal/hal_timer.h"),
    Path("basalt_hal/include/hal/hal_i2s.h"),
    Path("basalt_hal/include/hal/hal_rmt.h"),
]
for h in required_headers:
    if not h.exists():
        raise SystemExit(f"missing HAL header: {h}")

expected_symbols = {
    "spi": ["hal_spi_init(", "hal_spi_transfer(", "hal_spi_deinit("],
    "adc": ["hal_adc_init(", "hal_adc_read_raw(", "hal_adc_deinit("],
    "pwm": ["hal_pwm_init(", "hal_pwm_set_duty_percent(", "hal_pwm_deinit("],
    "timer": ["hal_timer_init(", "hal_timer_start(", "hal_timer_deinit("],
    "i2s": ["hal_i2s_diag_init(", "hal_i2s_diag_loopback(", "hal_i2s_diag_deinit("],
    "rmt": ["hal_rmt_init(", "hal_rmt_loopback(", "hal_rmt_deinit("],
}

# Per-target adapter checks for shared core primitives.
for port in ports:
    for prim in core_prims:
        p = Path(f"basalt_hal/ports/{port}/hal_{prim}.c")
        if not p.exists():
            raise SystemExit(f"missing HAL source: {p}")
        txt = p.read_text(encoding="utf-8")
        if "#pragma once" in txt:
            raise SystemExit(f"invalid HAL source (header-like placeholder): {p}")
        if "Portable UART contract used by BasaltOS." in txt:
            raise SystemExit(f"invalid HAL source (copied UART header text): {p}")
        for sym in expected_symbols[prim]:
            if sym not in txt:
                raise SystemExit(f"missing symbol {sym} in {p}")

# ESP32 platform-level adapters for advanced peripherals.
for prim in esp32_prims:
    p = Path(f"basalt_hal/ports/esp32/hal_{prim}.c")
    if not p.exists():
        raise SystemExit(f"missing HAL source: {p}")
    txt = p.read_text(encoding="utf-8")
    if "#pragma once" in txt:
        raise SystemExit(f"invalid HAL source (header-like placeholder): {p}")
    if "Portable UART contract used by BasaltOS." in txt:
        raise SystemExit(f"invalid HAL source (copied UART header text): {p}")
    for sym in expected_symbols[prim]:
        if sym not in txt:
            raise SystemExit(f"missing symbol {sym} in {p}")

print("PASS: esp32 HAL peripheral contract smoke checks")
PY
