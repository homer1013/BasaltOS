#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/hal_port_maturity_report_drift"
mkdir -p "$TMP"

GEN_JSON="$TMP/HAL_PORT_MATURITY_REPORT.generated.json"
GEN_MD="$TMP/HAL_PORT_MATURITY_REPORT.generated.md"

python3 tools/generate_hal_maturity_report.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD"

if ! diff -u docs/planning/HAL_PORT_MATURITY_REPORT.json "$GEN_JSON" >/tmp/hal_port_maturity_report_json.diff; then
  echo "FAIL: HAL maturity JSON drift detected."
  echo "Run: python3 tools/generate_hal_maturity_report.py"
  cat /tmp/hal_port_maturity_report_json.diff
  exit 1
fi

if ! diff -u docs/planning/HAL_PORT_MATURITY_REPORT.md "$GEN_MD" >/tmp/hal_port_maturity_report_md.diff; then
  echo "FAIL: HAL maturity MD drift detected."
  echo "Run: python3 tools/generate_hal_maturity_report.py"
  cat /tmp/hal_port_maturity_report_md.diff
  exit 1
fi

echo "PASS: HAL port maturity report drift checks passed"
