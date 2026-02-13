#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

from tools.configure import discover_modules, validate_required_module_pins

modules = discover_modules(Path("modules"))

# Positive case based on ESP32-C6 baseline pin profile.
board_ok = {
    "pins": {
        "i2s_bclk": 22,
        "i2s_ws": 23,
        "i2s_din": 1,
        "i2s_dout": 3,
        "rmt_tx": 2,
        "rmt_rx": 3,
        "mic_in": 1,
        "adc_in": 1,
        "uart_tx": 16,
        "uart_rx": 17,
    },
    "pin_definitions": {
        "i2s_bclk": {"alternatives": [22, 6, 7, 10]},
        "i2s_ws": {"alternatives": [23, 7, 6, 11]},
        "i2s_din": {"alternatives": [1, 2, 4, 5]},
        "i2s_dout": {"alternatives": [3, 4, 5, 8]},
        "rmt_tx": {"alternatives": [2, 3, 4, 5, 8, 10]},
        "rmt_rx": {"alternatives": [3, 2, 5, 4, 9, 11]},
        "mic_in": {"alternatives": [1, 2, 3, 4]},
        "adc_in": {"alternatives": [1, 2, 3, 4]},
        "uart_tx": {"alternatives": [16, 4, 5]},
        "uart_rx": {"alternatives": [17, 4, 5]},
    },
}

enabled = ["i2s", "mic", "rmt", "adc", "uart"]
driver_cfg_ok = {
    "i2s": {"mode": "tx_rx"},
    "mic": {"source": "adc"},
    "rmt": {"enable_tx": True, "enable_rx": True},
    "uart": {"mode": "tx_rx"},
}
errs_ok = validate_required_module_pins(modules, enabled, board_ok, driver_cfg_ok)
if errs_ok:
    raise SystemExit("unexpected validation errors in positive case: " + "; ".join(errs_ok))

# Negative case: mic_in unbound and adc_in outside allowed alternatives.
board_bad = {
    "pins": dict(board_ok["pins"], mic_in=-1, adc_in=99),
    "pin_definitions": board_ok["pin_definitions"],
}

driver_cfg_bad = {
    "i2s": {"mode": "tx_rx"},
    "mic": {"source": "adc"},
    "rmt": {"enable_tx": True, "enable_rx": True},
    "uart": {"mode": "tx_rx"},
}
errs_bad = validate_required_module_pins(modules, enabled, board_bad, driver_cfg_bad)
if not errs_bad:
    raise SystemExit("expected validation errors for negative case, got none")

expected_fragments = [
    "module 'mic' source=adc requires pin 'mic_in' to be bound",
    "module 'adc' pin 'adc_in' is outside board pin_definitions alternatives",
]
for frag in expected_fragments:
    if not any(frag in e for e in errs_bad):
        raise SystemExit(f"missing expected validation error fragment: {frag}; got: {errs_bad}")

# Negative UART constraint: tx/rx cannot share the same bound pin in tx_rx mode.
board_uart_bad = {
    "pins": dict(board_ok["pins"], uart_tx=16, uart_rx=16),
    "pin_definitions": board_ok["pin_definitions"],
}
errs_uart_bad = validate_required_module_pins(modules, enabled, board_uart_bad, driver_cfg_ok)
if not any("distinct pins for uart_tx and uart_rx" in e for e in errs_uart_bad):
    raise SystemExit("missing expected uart distinct-pin validation error")

# Positive UART mode constraint: rx-only should not require uart_tx.
board_uart_rx_only = {
    "pins": dict(board_ok["pins"], uart_tx=-1, uart_rx=17),
    "pin_definitions": board_ok["pin_definitions"],
}
driver_cfg_uart_rx_only = dict(driver_cfg_ok, uart={"mode": "rx"})
errs_uart_rx_only = validate_required_module_pins(modules, enabled, board_uart_rx_only, driver_cfg_uart_rx_only)
if errs_uart_rx_only:
    raise SystemExit("unexpected uart rx-only validation errors: " + "; ".join(errs_uart_rx_only))

print("PASS: configure pin validation smoke checks")
PY
