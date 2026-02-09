#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/tmp/bme280_smoke"
rm -rf "$OUT"
mkdir -p "$OUT"

cd "$ROOT"
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers bme280 \
  --outdir "$OUT" >/dev/null

python3 - <<'PY' "$OUT/basalt.features.json"
import json,sys
p=sys.argv[1]
d=json.load(open(p,'r',encoding='utf-8'))
mods=set(d.get('modules') or [])
assert 'bme280' in mods, 'bme280 missing from modules output'
print('modules_ok')
PY

grep -q "#define BASALT_ENABLE_BME280" "$OUT/basalt_config.h"

echo "PASS: bme280 configure smoke"
