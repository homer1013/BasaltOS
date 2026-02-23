#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUTDIR="$(mktemp -d /tmp/configure_tft_parallel_uno.XXXXXX)"
trap 'rm -rf "$OUTDIR"' EXIT

python3 tools/configure.py \
  --platform renesas_ra \
  --board arduino_uno_r4_wifi \
  --enable-drivers gpio,uart,shell_full,tft_parallel_uno \
  --outdir "$OUTDIR" >/tmp/configure_tft_parallel_uno.log 2>&1

CFG_H="$OUTDIR/basalt_config.h"
CFG_JSON="$OUTDIR/basalt_config.json"

if ! grep -q "#define BASALT_ENABLE_TFT_PARALLEL_UNO 1" "$CFG_H"; then
  echo "FAIL: BASALT_ENABLE_TFT_PARALLEL_UNO define missing from generated config"
  exit 1
fi
if ! grep -q "#define BASALT_PIN_LCD_D0 \"D8\"" "$CFG_H"; then
  echo "FAIL: expected UNO parallel TFT pin binding BASALT_PIN_LCD_D0"
  exit 1
fi
if ! grep -q '"tft_parallel_uno"' "$CFG_JSON"; then
  echo "FAIL: expected tft_parallel_uno in generated resolved drivers"
  exit 1
fi

echo "PASS: configure tft_parallel_uno smoke checks"
