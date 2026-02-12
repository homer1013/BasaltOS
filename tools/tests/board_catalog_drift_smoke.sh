#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/board_catalog"
mkdir -p "$TMP"
GEN="$TMP/BOARD_CATALOG.generated.md"

python3 tools/generate_board_catalog.py --out "$GEN" >/dev/null

if ! diff -u docs/BOARD_CATALOG.md "$GEN" >/dev/null; then
  echo "FAIL: docs/BOARD_CATALOG.md is out of date"
  echo "Run: python3 tools/generate_board_catalog.py"
  diff -u docs/BOARD_CATALOG.md "$GEN" || true
  exit 1
fi

echo "PASS: board catalog drift smoke checks passed"
