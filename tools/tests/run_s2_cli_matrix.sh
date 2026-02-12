#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

DATE="$(date +%F)"
OUT="${1:-tmp/acceptance/s2-cli/$DATE}"
ARTIFACTS="$OUT/artifacts"
LOG="$OUT/commands.log"
RESULTS="$OUT/matrix_results.md"

mkdir -p "$ARTIFACTS" "$OUT/cases"
: > "$LOG"

failures=0

cat > "$RESULTS" <<EOF
# Sprint 2 CLI Reliability Matrix Results

- Date: $DATE
- Output: \`$OUT\`

| ID | Status | Notes |
|---|---|---|
EOF

mark_result() {
  local id="$1"
  local status="$2"
  local notes="$3"
  echo "| $id | $status | $notes |" >> "$RESULTS"
}

run_expect_zero() {
  local id="$1"
  local cmd="$2"
  local case_log="$OUT/cases/${id}.log"

  {
    echo "### $id"
    echo "\$ $cmd"
  } >> "$LOG"

  set +e
  eval "$cmd" >"$case_log" 2>&1
  local rc=$?
  set -e

  cat "$case_log" >> "$LOG"
  echo >> "$LOG"

  if [[ "$rc" -eq 0 ]]; then
    mark_result "$id" "PASS" "exit=0"
  else
    mark_result "$id" "FAIL" "expected exit=0, got exit=$rc"
    failures=$((failures + 1))
  fi
}

run_expect_nonzero_with_msg() {
  local id="$1"
  local cmd="$2"
  local pattern="$3"
  local case_log="$OUT/cases/${id}.log"

  {
    echo "### $id"
    echo "\$ $cmd"
  } >> "$LOG"

  set +e
  eval "$cmd" >"$case_log" 2>&1
  local rc=$?
  set -e

  cat "$case_log" >> "$LOG"
  echo >> "$LOG"

  if [[ "$rc" -eq 0 ]]; then
    mark_result "$id" "FAIL" "expected non-zero exit, got exit=0"
    failures=$((failures + 1))
    return
  fi

  if command -v rg >/dev/null 2>&1; then
    if rg -qi "$pattern" "$case_log"; then
      mark_result "$id" "PASS" "exit=$rc and message matched /$pattern/"
    else
      mark_result "$id" "FAIL" "exit=$rc but missing message /$pattern/"
      failures=$((failures + 1))
    fi
  else
    if grep -Eiq "$pattern" "$case_log"; then
      mark_result "$id" "PASS" "exit=$rc and message matched /$pattern/"
    else
      mark_result "$id" "FAIL" "exit=$rc but missing message /$pattern/"
      failures=$((failures + 1))
    fi
  fi
}

require_generated_set() {
  local id="$1"
  local dir="$2"
  local missing=()
  local files=(
    "basalt_config.h"
    "basalt.features.json"
    "basalt_config.json"
    "sdkconfig.defaults"
  )

  for f in "${files[@]}"; do
    if [[ ! -f "$dir/$f" ]]; then
      missing+=("$f")
    fi
  done

  if [[ "${#missing[@]}" -eq 0 ]]; then
    mark_result "$id-artifacts" "PASS" "required generated files present"
  else
    mark_result "$id-artifacts" "FAIL" "missing: ${missing[*]}"
    failures=$((failures + 1))
  fi
}

run_expect_zero "CLI-01" "python3 tools/configure.py --list-boards"
run_expect_zero "CLI-02" "python3 tools/configure.py --list-drivers"
run_expect_zero "CLI-03" "python3 tools/configure.py --help"

run_expect_zero "CLI-04" "python3 tools/configure.py --platform esp32 --board esp32-c3-supermini --enable-drivers uart --outdir \"$ARTIFACTS/esp32\""
require_generated_set "CLI-04" "$ARTIFACTS/esp32"

run_expect_zero "CLI-05" "python3 tools/configure.py --platform stm32 --board nucleo_f401re --enable-drivers uart --outdir \"$ARTIFACTS/stm32\""
require_generated_set "CLI-05" "$ARTIFACTS/stm32"

run_expect_zero "CLI-06" "python3 tools/configure.py --platform rp2040 --board raspberry_pi_pico --enable-drivers uart --outdir \"$ARTIFACTS/rp2040\""
require_generated_set "CLI-06" "$ARTIFACTS/rp2040"

run_expect_zero "CLI-07" "python3 tools/configure.py --platform atmega --board arduino_uno_r3 --enable-drivers uart --outdir \"$ARTIFACTS/avr\""
require_generated_set "CLI-07" "$ARTIFACTS/avr"

run_expect_nonzero_with_msg "CLI-08" "python3 tools/configure.py --platform esp32 --board does-not-exist --enable-drivers uart" "board .*not found|unknown board|available boards"
run_expect_zero "CLI-08b" "bash tools/tests/configure_negative_paths.sh"
run_expect_zero "CLI-09" "bash -n tools/board.sh"
run_expect_zero "CLI-10" "bash tools/board.sh --list"
run_expect_zero "CLI-11" "bash tools/tests/configure_deterministic_outputs.sh"
run_expect_zero "CLI-12" "bash tools/tests/configure_smoke_multi_board.sh"

if [[ "$failures" -eq 0 ]]; then
  echo >> "$RESULTS"
  echo "PASS: all Sprint 2 CLI matrix checks passed." >> "$RESULTS"
  echo "PASS: all Sprint 2 CLI matrix checks passed."
  echo "Artifacts: $OUT"
  exit 0
fi

echo >> "$RESULTS"
echo "FAIL: $failures check(s) failed." >> "$RESULTS"
echo "FAIL: $failures check(s) failed. See $RESULTS and $LOG"
exit 1
