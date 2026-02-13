#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/hal_platform_adapter_completeness_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_JSON="$TMP/HAL_PLATFORM_ADAPTER_COMPLETENESS.generated.json"
GEN_MD="$TMP/HAL_PLATFORM_ADAPTER_COMPLETENESS.generated.md"

python3 tools/generate_hal_platform_adapter_completeness.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" >/dev/null

if ! diff -u docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json is out of date"
  echo "Run: python3 tools/generate_hal_platform_adapter_completeness.py"
  diff -u docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.md is out of date"
  echo "Run: python3 tools/generate_hal_platform_adapter_completeness.py"
  diff -u docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.md "$GEN_MD" || true
  exit 1
fi

python3 - <<'PY'
import json
from pathlib import Path

payload = json.loads(Path("docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json").read_text(encoding="utf-8"))
summary = payload.get("summary") or {}
blocking = int(summary.get("blocking_gaps", 0))
if blocking != 0:
    raise SystemExit(f"FAIL: HAL platform adapter completeness has {blocking} blocking gap(s)")
print("PASS: HAL platform adapter completeness smoke checks passed")
PY
