#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_ROOT="$(mktemp -d /tmp/non_esp_hero_bringup.XXXXXX)"
DRIVERS="gpio,uart,i2c,spi,timer,pwm,shell_full"

check_board() {
  local platform="$1"
  local board="$2"
  local outdir="$OUT_ROOT/$platform"

  python3 tools/configure.py \
    --platform "$platform" \
    --board "$board" \
    --enable-drivers "$DRIVERS" \
    --outdir "$outdir" >/tmp/non_esp_hero_"$platform".log 2>&1

  local hdr="$outdir/basalt_config.h"
  local cfg="$outdir/basalt_config.json"
  local feat="$outdir/basalt.features.json"

  [[ -f "$hdr" ]] || { echo "FAIL: missing $hdr"; exit 1; }
  [[ -f "$cfg" ]] || { echo "FAIL: missing $cfg"; exit 1; }
  [[ -f "$feat" ]] || { echo "FAIL: missing $feat"; exit 1; }

  grep -q "BASALT_ENABLE_GPIO 1" "$hdr" || { echo "FAIL: gpio define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_UART 1" "$hdr" || { echo "FAIL: uart define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_I2C 1" "$hdr" || { echo "FAIL: i2c define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_SPI 1" "$hdr" || { echo "FAIL: spi define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_TIMER 1" "$hdr" || { echo "FAIL: timer define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_PWM 1" "$hdr" || { echo "FAIL: pwm define missing for $platform"; exit 1; }
  grep -q "BASALT_ENABLE_SHELL_FULL 1" "$hdr" || { echo "FAIL: shell_full define missing for $platform"; exit 1; }
}

check_board "rp2040" "raspberry_pi_pico"
check_board "stm32" "nucleo_f446re"
check_board "pic16" "curiosity_nano_pic16f13145"

python3 - "$OUT_ROOT" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
rows = []
for platform, board in [
    ("rp2040", "raspberry_pi_pico"),
    ("stm32", "nucleo_f446re"),
    ("pic16", "curiosity_nano_pic16f13145"),
]:
    cfg = json.loads((root / platform / "basalt_config.json").read_text(encoding="utf-8"))
    rows.append(
        {
            "platform": platform,
            "board": board,
            "target_profile": str(cfg.get("target_profile") or ""),
            "enabled_drivers": list(cfg.get("enabled_drivers") or []),
            "status": "PASS",
        }
    )

print(json.dumps({"rows": rows}, indent=2))
PY

echo "PASS: non-ESP hero bring-up smoke checks passed"
