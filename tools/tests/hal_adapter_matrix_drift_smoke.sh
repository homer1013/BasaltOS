#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/hal_adapter_matrix_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_CSV="$TMP/HAL_ADAPTER_MATRIX.generated.csv"
GEN_JSON="$TMP/HAL_ADAPTER_MATRIX.generated.json"
GEN_MD="$TMP/HAL_ADAPTER_MATRIX.generated.md"

python3 tools/generate_hal_adapter_matrix.py \
  --csv-out "$GEN_CSV" \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" >/dev/null

if ! diff -u docs/planning/HAL_ADAPTER_MATRIX.csv "$GEN_CSV" >/dev/null; then
  echo "FAIL: docs/planning/HAL_ADAPTER_MATRIX.csv is out of date"
  echo "Run: python3 tools/generate_hal_adapter_matrix.py"
  diff -u docs/planning/HAL_ADAPTER_MATRIX.csv "$GEN_CSV" || true
  exit 1
fi

if ! diff -u docs/planning/HAL_ADAPTER_MATRIX.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/HAL_ADAPTER_MATRIX.json is out of date"
  echo "Run: python3 tools/generate_hal_adapter_matrix.py"
  diff -u docs/planning/HAL_ADAPTER_MATRIX.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/HAL_ADAPTER_MATRIX.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/HAL_ADAPTER_MATRIX.md is out of date"
  echo "Run: python3 tools/generate_hal_adapter_matrix.py"
  diff -u docs/planning/HAL_ADAPTER_MATRIX.md "$GEN_MD" || true
  exit 1
fi

echo "PASS: hal adapter matrix drift smoke checks passed"
