#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP_DIR="tmp/test_configure_negative_paths"
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

run_fail_case() {
  local name="$1"
  local expect_rc="$2"
  local expect_msg="$3"
  shift 3

  local log="$TMP_DIR/${name}.log"
  set +e
  python3 tools/configure.py "$@" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ "$rc" -ne "$expect_rc" ]]; then
    echo "FAIL: $name expected rc=$expect_rc, got rc=$rc"
    echo "---- $log ----"
    cat "$log"
    exit 1
  fi

  if ! has_msg "$expect_msg" "$log"; then
    echo "FAIL: $name expected message '$expect_msg'"
    echo "---- $log ----"
    cat "$log"
    exit 1
  fi
}

# 1) Cross-platform board mismatch should hard fail.
run_fail_case \
  "board_platform_mismatch" \
  3 \
  "not found under platform" \
  --platform stm32 \
  --board esp32-c3-supermini \
  --outdir "$TMP_DIR/mismatch_out"

if [[ -d "$TMP_DIR/mismatch_out" ]]; then
  echo "FAIL: board-platform mismatch should not create output dir"
  exit 1
fi

# 2) Unknown template id should fail with explicit guidance.
run_fail_case \
  "unknown_template" \
  3 \
  "unknown template" \
  --platform esp32 \
  --template does-not-exist \
  --outdir "$TMP_DIR/unknown_template_out"

# 3) Template not allowed for selected platform should fail.
run_fail_case \
  "template_platform_mismatch" \
  3 \
  "does not support platform" \
  --platform atmega \
  --template display_app \
  --outdir "$TMP_DIR/template_platform_mismatch_out"

# 4) Unknown driver should fail and list unknown driver id.
run_fail_case \
  "unknown_driver" \
  4 \
  "Unknown driver" \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers not_a_driver \
  --outdir "$TMP_DIR/unknown_driver_out"

# 5) Invalid AVR clock input should fail validation.
run_fail_case \
  "invalid_clock" \
  3 \
  "clock-hz must be > 0" \
  --platform atmega \
  --clock-hz 0 \
  --outdir "$TMP_DIR/invalid_clock_out"

echo "PASS: configure negative-path validation checks"
