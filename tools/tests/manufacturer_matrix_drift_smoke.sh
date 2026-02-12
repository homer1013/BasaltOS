#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/manufacturer_matrix_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_CSV="$TMP/MANUFACTURER_BOARD_MATRIX.generated.csv"
GEN_MD="$TMP/MANUFACTURER_BOARD_MATRIX.generated.md"
GEN_Q_JSON="$TMP/MANUFACTURER_PROFILE_CREATION_QUEUE.generated.json"
GEN_Q_MD="$TMP/MANUFACTURER_PROFILE_CREATION_QUEUE.generated.md"

python3 tools/generate_manufacturer_matrix.py \
  --csv-out "$GEN_CSV" \
  --md-out "$GEN_MD" \
  --queue-json-out "$GEN_Q_JSON" \
  --queue-md-out "$GEN_Q_MD" >/dev/null

if ! diff -u docs/planning/MANUFACTURER_BOARD_MATRIX.csv "$GEN_CSV" >/dev/null; then
  echo "FAIL: docs/planning/MANUFACTURER_BOARD_MATRIX.csv is out of date"
  echo "Run: python3 tools/generate_manufacturer_matrix.py"
  diff -u docs/planning/MANUFACTURER_BOARD_MATRIX.csv "$GEN_CSV" || true
  exit 1
fi

if ! diff -u docs/planning/MANUFACTURER_BOARD_MATRIX.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/MANUFACTURER_BOARD_MATRIX.md is out of date"
  echo "Run: python3 tools/generate_manufacturer_matrix.py"
  diff -u docs/planning/MANUFACTURER_BOARD_MATRIX.md "$GEN_MD" || true
  exit 1
fi

if ! diff -u docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.json "$GEN_Q_JSON" >/dev/null; then
  echo "FAIL: docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.json is out of date"
  echo "Run: python3 tools/generate_manufacturer_matrix.py"
  diff -u docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.json "$GEN_Q_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.md "$GEN_Q_MD" >/dev/null; then
  echo "FAIL: docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.md is out of date"
  echo "Run: python3 tools/generate_manufacturer_matrix.py"
  diff -u docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.md "$GEN_Q_MD" || true
  exit 1
fi

echo "PASS: manufacturer matrix drift smoke checks passed"
