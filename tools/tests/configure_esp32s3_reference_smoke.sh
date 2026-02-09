#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/tmp/esp32s3_reference_smoke"
rm -rf "$OUT"
mkdir -p "$OUT"

cd "$ROOT"
python3 tools/configure.py \
  --platform esp32 \
  --board esp32s3_devkitc_1 \
  --enable-drivers uart,spi,i2c,mcp2515,bme280,shell_full \
  --outdir "$OUT" >/dev/null

python3 - <<'PY' "$OUT/basalt.features.json"
import json,sys
p=sys.argv[1]
d=json.load(open(p,'r',encoding='utf-8'))
mods=set(d.get('modules') or [])
assert 'mcp2515' in mods, 'mcp2515 missing from modules'
assert 'bme280' in mods, 'bme280 missing from modules'
assert d.get('board_id') == 'esp32s3_devkitc_1', f"unexpected board_id: {d.get('board_id')}"
print('features_ok')
PY

grep -q "#define BASALT_ENABLE_MCP2515 1" "$OUT/basalt_config.h"
grep -q "#define BASALT_ENABLE_BME280 1" "$OUT/basalt_config.h"
grep -q "#define BASALT_PIN_CAN_CS " "$OUT/basalt_config.h"
grep -q "#define BASALT_PIN_CAN_INT " "$OUT/basalt_config.h"
grep -q "#define BASALT_PIN_SPI_SCLK " "$OUT/basalt_config.h"
grep -q "#define BASALT_PIN_SPI_MISO " "$OUT/basalt_config.h"
grep -q "#define BASALT_PIN_SPI_MOSI " "$OUT/basalt_config.h"

echo "PASS: esp32s3 reference smoke"
