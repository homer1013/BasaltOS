#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/hal_runtime_contract_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_JSON="$TMP/HAL_RUNTIME_CONTRACT_REPORT.generated.json"
GEN_MD="$TMP/HAL_RUNTIME_CONTRACT_REPORT.generated.md"

python3 tools/generate_hal_runtime_contract_report.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" >/dev/null

if ! diff -u docs/planning/HAL_RUNTIME_CONTRACT_REPORT.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/HAL_RUNTIME_CONTRACT_REPORT.json is out of date"
  echo "Run: python3 tools/generate_hal_runtime_contract_report.py"
  diff -u docs/planning/HAL_RUNTIME_CONTRACT_REPORT.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/HAL_RUNTIME_CONTRACT_REPORT.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/HAL_RUNTIME_CONTRACT_REPORT.md is out of date"
  echo "Run: python3 tools/generate_hal_runtime_contract_report.py"
  diff -u docs/planning/HAL_RUNTIME_CONTRACT_REPORT.md "$GEN_MD" || true
  exit 1
fi

python3 - <<'PY'
import json
from pathlib import Path

policy = json.loads(Path("docs/planning/HAL_RUNTIME_CONTRACT_POLICY.json").read_text(encoding="utf-8"))
report = json.loads(Path("docs/planning/HAL_RUNTIME_CONTRACT_REPORT.json").read_text(encoding="utf-8"))

tracked_policy = list(policy.get("tracked_primitives") or [])
tracked_report = list(report.get("tracked_primitives") or [])
if tracked_policy != tracked_report:
    raise SystemExit("FAIL: runtime contract tracked_primitives mismatch between policy and report")

summary = report.get("summary") or {}
blocking = int(summary.get("blocking_gaps", 0))
if blocking != 0:
    raise SystemExit(f"FAIL: HAL runtime contract has {blocking} blocking gap(s)")

print("PASS: HAL runtime contract smoke checks passed")
PY
