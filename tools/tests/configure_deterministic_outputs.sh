#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_BASE="tmp/test_configure_deterministic"
rm -rf "$OUT_BASE"
mkdir -p "$OUT_BASE"

pairs=(
  "esp32|esp32-c3-supermini"
  "stm32|nucleo_f401re"
  "rp2040|raspberry_pi_pico"
  "atmega|arduino_uno_r3"
)

normalize_json() {
  local in_file="$1"
  local out_file="$2"
  python3 - "$in_file" "$out_file" <<'PY'
import json,sys
src,dst=sys.argv[1],sys.argv[2]
obj=json.load(open(src,'r',encoding='utf-8'))
if isinstance(obj,dict):
    obj.pop('generated_utc',None)
json.dump(obj,open(dst,'w',encoding='utf-8'),indent=2,sort_keys=True)
print()
PY
}

for pair in "${pairs[@]}"; do
  IFS='|' read -r platform board <<< "$pair"
  r1="$OUT_BASE/one/${platform}/${board}"
  r2="$OUT_BASE/two/${platform}/${board}"
  mkdir -p "$r1" "$r2"

  echo "[deterministic] ${platform}/${board}"
  python3 tools/configure.py --platform "$platform" --board "$board" --enable-drivers uart --outdir "$r1" >"$r1/run.log" 2>&1
  sleep 1
  python3 tools/configure.py --platform "$platform" --board "$board" --enable-drivers uart --outdir "$r2" >"$r2/run.log" 2>&1

  diff -u "$r1/basalt_config.h" "$r2/basalt_config.h" >/dev/null
  diff -u "$r1/sdkconfig.defaults" "$r2/sdkconfig.defaults" >/dev/null

  normalize_json "$r1/basalt.features.json" "$r1/basalt.features.norm.json"
  normalize_json "$r2/basalt.features.json" "$r2/basalt.features.norm.json"
  diff -u "$r1/basalt.features.norm.json" "$r2/basalt.features.norm.json" >/dev/null

  normalize_json "$r1/basalt_config.json" "$r1/basalt_config.norm.json"
  normalize_json "$r2/basalt_config.json" "$r2/basalt_config.norm.json"
  diff -u "$r1/basalt_config.norm.json" "$r2/basalt_config.norm.json" >/dev/null

done

echo "PASS: deterministic output checks passed (normalized for generated_utc)"
