#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP_JSON="$(mktemp)"
TMP_MD="$(mktemp)"
trap 'rm -f "$TMP_JSON" "$TMP_MD"' EXIT

python3 tools/generate_hal_unsupported_stub_inventory.py \
  --json-out "$TMP_JSON" \
  --md-out "$TMP_MD" >/dev/null

if ! diff -u docs/planning/HAL_UNSUPPORTED_STUB_INVENTORY.json "$TMP_JSON" >/dev/null; then
  echo "FAIL: HAL unsupported stub inventory JSON drift detected"
  echo "Run: python3 tools/generate_hal_unsupported_stub_inventory.py"
  exit 1
fi

if ! diff -u docs/planning/HAL_UNSUPPORTED_STUB_INVENTORY.md "$TMP_MD" >/dev/null; then
  echo "FAIL: HAL unsupported stub inventory Markdown drift detected"
  echo "Run: python3 tools/generate_hal_unsupported_stub_inventory.py"
  exit 1
fi

echo "PASS: HAL unsupported stub inventory drift smoke checks passed"
