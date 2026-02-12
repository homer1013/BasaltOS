#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/board_catalog"
mkdir -p "$TMP"
GEN="$TMP/BOARD_CATALOG.generated.md"
GEN_JSON="$TMP/BOARD_TAXONOMY_INDEX.generated.json"
GEN_CSV="$TMP/BOARD_TAXONOMY_INDEX.generated.csv"

python3 tools/generate_board_catalog.py --out "$GEN" --json-out "$GEN_JSON" --csv-out "$GEN_CSV" >/dev/null

if ! diff -u docs/BOARD_CATALOG.md "$GEN" >/dev/null; then
  echo "FAIL: docs/BOARD_CATALOG.md is out of date"
  echo "Run: python3 tools/generate_board_catalog.py"
  diff -u docs/BOARD_CATALOG.md "$GEN" || true
  exit 1
fi

if ! diff -u docs/BOARD_TAXONOMY_INDEX.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/BOARD_TAXONOMY_INDEX.json is out of date"
  echo "Run: python3 tools/generate_board_catalog.py"
  diff -u docs/BOARD_TAXONOMY_INDEX.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/BOARD_TAXONOMY_INDEX.csv "$GEN_CSV" >/dev/null; then
  echo "FAIL: docs/BOARD_TAXONOMY_INDEX.csv is out of date"
  echo "Run: python3 tools/generate_board_catalog.py"
  diff -u docs/BOARD_TAXONOMY_INDEX.csv "$GEN_CSV" || true
  exit 1
fi

echo "PASS: board catalog drift smoke checks passed"
