#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
import importlib.util
import sys
import tempfile
from pathlib import Path

root = Path.cwd()
spec = importlib.util.spec_from_file_location("configure_mod", root / "tools" / "configure.py")
cfg = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = cfg
spec.loader.exec_module(cfg)

mods = cfg.discover_modules(root / "modules")
enabled = ["gpio", "i2s", "mic", "rmt"]
driver_config = {"rmt": {"enable_rx": True}}

with tempfile.TemporaryDirectory(prefix="cfg-defaults-") as td:
    out = Path(td) / "basalt_config.h"
    out.write_text("/* smoke */\n", encoding="utf-8")
    cfg.append_driver_config_macros(out, mods, enabled, driver_config)
    text = out.read_text(encoding="utf-8")

required = [
    "#define BASALT_CFG_I2S_SAMPLE_RATE 16000",
    "#define BASALT_CFG_I2S_BITS_PER_SAMPLE 16",
    "#define BASALT_CFG_I2S_MODE \"rx\"",
    "#define BASALT_CFG_MIC_SOURCE \"adc\"",
    "#define BASALT_CFG_MIC_SAMPLE_RATE 16000",
    "#define BASALT_CFG_RMT_RESOLUTION_HZ 1000000",
    "#define BASALT_CFG_RMT_ENABLE_TX 1",
    "#define BASALT_CFG_RMT_ENABLE_RX 1",
    "#define BASALT_CFG_RMT_ENABLE_RX_CHANNEL 1",
]
for token in required:
    if token not in text:
        raise SystemExit(f"missing expected macro: {token}")

if "BASALT_CFG_UART_UART_BAUDRATE" in text:
    raise SystemExit("unexpected macro emitted for disabled module 'uart'")

print("PASS: configure driver defaults smoke checks")
PY
