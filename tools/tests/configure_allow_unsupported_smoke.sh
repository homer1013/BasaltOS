#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

tmp_no="$(mktemp)"
tmp_yes="$(mktemp)"
trap 'rm -f "$tmp_no" "$tmp_yes"' EXIT

set +e
python3 tools/configure.py \
  --platform renesas_ra \
  --board arduino_uno_r4_wifi \
  --enable-drivers gpio,uart,shell_full,tft \
  --outdir /tmp/configure_allow_unsupported_no >"$tmp_no" 2>&1
rc_no=$?

python3 tools/configure.py \
  --platform renesas_ra \
  --board arduino_uno_r4_wifi \
  --enable-drivers gpio,uart,shell_full,tft \
  --allow-unsupported \
  --outdir /tmp/configure_allow_unsupported_yes >"$tmp_yes" 2>&1
rc_yes=$?
set -e

if [[ $rc_no -eq 0 ]]; then
  echo "FAIL: expected configure without override to fail for unsupported driver"
  exit 1
fi
if [[ $rc_yes -eq 0 ]]; then
  echo "FAIL: expected configure with override to still fail (missing board pin contract)"
  exit 1
fi

if ! grep -q "Driver 'tft' is not supported by selected board/capabilities" "$tmp_no"; then
  echo "FAIL: expected unsupported-driver error without override"
  exit 1
fi
if ! grep -q -- "--allow-unsupported enabled; bypassing board capability restriction" "$tmp_yes"; then
  echo "FAIL: expected override warning with --allow-unsupported"
  exit 1
fi
if ! grep -q "requires pin 'tft_mosi'" "$tmp_yes"; then
  echo "FAIL: expected next-layer pin-contract error with override"
  exit 1
fi

echo "PASS: configure --allow-unsupported smoke checks"
