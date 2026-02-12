#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/manufacturer_candidate_smoke"
rm -rf "$OUT"
mkdir -p "$OUT"

declare -a CANDIDATES=(
  "Adafruit|atsam|adafruit_circuit_playground_express|bench"
  "Adafruit|rp2040|adafruit_feather_rp2040|metadata"
  "Seeed Studio|atsam|seeeduino_xiao_samd21|metadata"
  "SparkFun|rp2040|sparkfun_thing_plus_rp2040|planned"
  "DF Robot|esp32|dfrobot_firebeetle_esp32|planned"
  "DF Robot|esp32|dfrobot_beetle_esp32_c3|metadata"
  "Elecrow|esp32|elecrow_crowpanel_esp32|planned"
  "Elecrow|esp32|elecrow_crowpanel_esp32_s3|metadata"
)

pass=0
missing=0
failed=0

{
  echo "# Manufacturer Candidate Validation"
  echo
  echo "| Manufacturer | Candidate | Status | Lane | Notes |"
  echo "|---|---|---|---|---|"
} > "$OUT/summary.md"

for row in "${CANDIDATES[@]}"; do
  IFS='|' read -r manu platform board lane <<< "$row"
  board_file="boards/$platform/$board/board.json"
  if [[ ! -f "$board_file" ]]; then
    echo "| $manu | $platform/$board | MISSING_PROFILE | $lane | planned backlog candidate (no board.json yet) |" >> "$OUT/summary.md"
    missing=$((missing + 1))
    continue
  fi

  run_dir="$OUT/$platform/$board"
  mkdir -p "$run_dir"
  if python3 tools/configure.py --platform "$platform" --board "$board" --outdir "$run_dir" >"$run_dir/run.log" 2>&1; then
    test -f "$run_dir/basalt_config.h"
    test -f "$run_dir/basalt.features.json"
    test -f "$run_dir/sdkconfig.defaults"
    echo "| $manu | $platform/$board | PASS | $lane | configure smoke passed |" >> "$OUT/summary.md"
    pass=$((pass + 1))
  else
    echo "| $manu | $platform/$board | FAIL | $lane | configure failed (see $run_dir/run.log) |" >> "$OUT/summary.md"
    failed=$((failed + 1))
  fi
done

{
  echo
  echo "Summary: pass=$pass missing_profile=$missing failed=$failed"
} >> "$OUT/summary.md"

cat "$OUT/summary.md"

if [[ "$failed" -ne 0 ]]; then
  echo "FAIL: manufacturer candidate validation had failures"
  exit 1
fi

echo "PASS: manufacturer candidate validation smoke (missing profiles tracked as planned)"
