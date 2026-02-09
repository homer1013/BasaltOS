#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$ROOT/tmp/platformio_phase1_contract"
GEN="$TMP/generated"
OUT="$TMP/bootstrap"

rm -rf "$TMP"
mkdir -p "$GEN" "$OUT"

cd "$ROOT"

python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uart \
  --outdir "$GEN" >/dev/null

python3 -m py_compile tools/platformio/bootstrap_from_features.py
python3 tools/platformio/bootstrap_from_features.py \
  --features "$GEN/basalt.features.json" \
  --sdk-defaults "$GEN/sdkconfig.defaults" \
  --outdir "$OUT" >/dev/null

[[ -f "$OUT/platformio.ini" ]]
[[ -f "$OUT/README.md" ]]

grep -q '^platform = espressif32$' "$OUT/platformio.ini"
grep -q '^framework = espidf$' "$OUT/platformio.ini"
grep -q '^board_build.sdkconfig_defaults =$' "$OUT/platformio.ini"
grep -q 'config/generated/sdkconfig.defaults' "$OUT/platformio.ini" || grep -q 'generated/sdkconfig.defaults' "$OUT/platformio.ini"
grep -q '^# BasaltOS PlatformIO Phase-1 Bootstrap$' "$OUT/README.md"

echo "PASS: platformio phase-1 contract smoke"
