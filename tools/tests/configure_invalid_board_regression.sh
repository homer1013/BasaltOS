#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP_DIR="tmp/test_configure_invalid_board"
BAD_OUT="$TMP_DIR/bad_out"
GOOD_OUT="$TMP_DIR/good_out"
LOG_BAD="$TMP_DIR/invalid.log"
LOG_GOOD="$TMP_DIR/valid.log"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

set +e
python3 tools/configure.py \
  --platform esp32 \
  --board does-not-exist \
  --enable-drivers uart \
  --outdir "$BAD_OUT" \
  >"$LOG_BAD" 2>&1
rc_bad=$?
set -e

if [[ "$rc_bad" -eq 0 ]]; then
  echo "FAIL: expected non-zero exit for invalid board, got 0"
  exit 1
fi

if ! rg -q "not found under platform" "$LOG_BAD"; then
  echo "FAIL: expected invalid-board error message not found"
  exit 1
fi

if [[ -d "$BAD_OUT" ]]; then
  echo "FAIL: invalid board should not generate output directory: $BAD_OUT"
  exit 1
fi

python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uart \
  --outdir "$GOOD_OUT" \
  >"$LOG_GOOD" 2>&1

for f in basalt_config.h basalt.features.json sdkconfig.defaults; do
  if [[ ! -f "$GOOD_OUT/$f" ]]; then
    echo "FAIL: expected generated file missing: $GOOD_OUT/$f"
    exit 1
  fi
done

echo "PASS: invalid-board regression guard and valid-board sanity check"
