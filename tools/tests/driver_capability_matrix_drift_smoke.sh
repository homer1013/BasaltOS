#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/driver_capability_matrix_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_CSV="$TMP/DRIVER_CAPABILITY_MATRIX.generated.csv"
GEN_JSON="$TMP/DRIVER_CAPABILITY_MATRIX.generated.json"
GEN_MD="$TMP/DRIVER_CAPABILITY_MATRIX.generated.md"
GEN_GAP_JSON="$TMP/DRIVER_CAPABILITY_GAPS.generated.json"
GEN_GAP_MD="$TMP/DRIVER_CAPABILITY_GAPS.generated.md"

python3 tools/generate_driver_capability_matrix.py \
  --csv-out "$GEN_CSV" \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" \
  --gap-json-out "$GEN_GAP_JSON" \
  --gap-md-out "$GEN_GAP_MD" >/dev/null

if ! diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.csv "$GEN_CSV" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_CAPABILITY_MATRIX.csv is out of date"
  echo "Run: python3 tools/generate_driver_capability_matrix.py"
  diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.csv "$GEN_CSV" || true
  exit 1
fi

if ! diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_CAPABILITY_MATRIX.json is out of date"
  echo "Run: python3 tools/generate_driver_capability_matrix.py"
  diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_CAPABILITY_MATRIX.md is out of date"
  echo "Run: python3 tools/generate_driver_capability_matrix.py"
  diff -u docs/planning/DRIVER_CAPABILITY_MATRIX.md "$GEN_MD" || true
  exit 1
fi

if ! diff -u docs/planning/DRIVER_CAPABILITY_GAPS.json "$GEN_GAP_JSON" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_CAPABILITY_GAPS.json is out of date"
  echo "Run: python3 tools/generate_driver_capability_matrix.py"
  diff -u docs/planning/DRIVER_CAPABILITY_GAPS.json "$GEN_GAP_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/DRIVER_CAPABILITY_GAPS.md "$GEN_GAP_MD" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_CAPABILITY_GAPS.md is out of date"
  echo "Run: python3 tools/generate_driver_capability_matrix.py"
  diff -u docs/planning/DRIVER_CAPABILITY_GAPS.md "$GEN_GAP_MD" || true
  exit 1
fi

echo "PASS: driver capability matrix drift smoke checks passed"
