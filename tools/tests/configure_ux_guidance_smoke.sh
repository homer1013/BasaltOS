#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP_DIR="tmp/test_configure_ux_guidance"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

has_msg() {
  local pattern="$1"
  local file="$2"
  if command -v rg >/dev/null 2>&1; then
    rg -q "$pattern" "$file"
  else
    grep -q "$pattern" "$file"
  fi
}

# 1) Invalid board should include closest-match guidance.
set +e
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-superminix \
  --enable-drivers uart \
  --outdir "$TMP_DIR/invalid_board_out" \
  >"$TMP_DIR/invalid_board.log" 2>&1
rc1=$?
set -e
if [[ "$rc1" -ne 3 ]]; then
  echo "FAIL: invalid-board case expected rc=3, got $rc1"
  cat "$TMP_DIR/invalid_board.log"
  exit 1
fi
has_msg "Closest matches:" "$TMP_DIR/invalid_board.log" || { echo "FAIL: missing closest-match guidance"; cat "$TMP_DIR/invalid_board.log"; exit 1; }

# 2) Unknown driver should include suggestion/availability guidance.
set +e
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uar \
  --outdir "$TMP_DIR/unknown_driver_out" \
  >"$TMP_DIR/unknown_driver.log" 2>&1
rc2=$?
set -e
if [[ "$rc2" -ne 4 ]]; then
  echo "FAIL: unknown-driver case expected rc=4, got $rc2"
  cat "$TMP_DIR/unknown_driver.log"
  exit 1
fi
has_msg "Unknown driver" "$TMP_DIR/unknown_driver.log" || { echo "FAIL: missing unknown-driver error"; cat "$TMP_DIR/unknown_driver.log"; exit 1; }
if ! has_msg "did you mean|Available drivers \\(sample\\)" "$TMP_DIR/unknown_driver.log"; then
  echo "FAIL: missing unknown-driver guidance"
  cat "$TMP_DIR/unknown_driver.log"
  exit 1
fi

# 3) Successful generation should print board-default guidance.
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uart \
  --outdir "$TMP_DIR/success_out" \
  >"$TMP_DIR/success.log" 2>&1

has_msg "Board default drivers:" "$TMP_DIR/success.log" || { echo "FAIL: missing board default guidance on success"; cat "$TMP_DIR/success.log"; exit 1; }

echo "PASS: configure UX guidance smoke checks passed"
