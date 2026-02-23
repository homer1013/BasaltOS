#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]

for port in ports:
    path = Path(f"basalt_hal/ports/{port}/hal_gpio.c")
    text = path.read_text(encoding="utf-8")
    required = [
        "static bool s_isr_service_installed = false;",
        "if (!s_isr_service_installed) {",
        "gpio_install_isr_service(0);",
        "ie != ESP_OK && ie != ESP_ERR_INVALID_STATE",
        "s_isr_service_installed = true;",
        "gpio_isr_handler_remove((gpio_num_t)g->pin);",
        "gpio_set_intr_type((gpio_num_t)g->pin, GPIO_INTR_DISABLE);",
    ]
    for needle in required:
        if needle not in text:
            raise SystemExit(f"FAIL: missing GPIO IRQ service contract '{needle}' in {path}")

print("PASS: HAL GPIO IRQ service contract smoke checks")
PY
