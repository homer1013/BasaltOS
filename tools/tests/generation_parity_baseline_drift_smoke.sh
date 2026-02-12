#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/generation_parity_baseline_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_JSON="$TMP/GENERATION_PARITY_BASELINE.generated.json"
GEN_MD="$TMP/GENERATION_PARITY_BASELINE.generated.md"

python3 tools/generate_generation_parity_baseline.py --out-json "$GEN_JSON" --out-md "$GEN_MD" >/dev/null

if ! diff -u docs/planning/GENERATION_PARITY_BASELINE.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/GENERATION_PARITY_BASELINE.json is out of date"
  echo "Run: python3 tools/generate_generation_parity_baseline.py"
  diff -u docs/planning/GENERATION_PARITY_BASELINE.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/GENERATION_PARITY_BASELINE.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/GENERATION_PARITY_BASELINE.md is out of date"
  echo "Run: python3 tools/generate_generation_parity_baseline.py"
  diff -u docs/planning/GENERATION_PARITY_BASELINE.md "$GEN_MD" || true
  exit 1
fi

echo "PASS: generation parity baseline drift smoke checks passed"
