#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/driver_hal_dependency_map_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_JSON="$TMP/DRIVER_HAL_DEPENDENCY_MAP.generated.json"
GEN_MD="$TMP/DRIVER_HAL_DEPENDENCY_MAP.generated.md"

python3 tools/generate_driver_hal_dependency_map.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" >/dev/null

if ! diff -u docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json is out of date"
  echo "Run: python3 tools/generate_driver_hal_dependency_map.py"
  diff -u docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/DRIVER_HAL_DEPENDENCY_MAP.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/DRIVER_HAL_DEPENDENCY_MAP.md is out of date"
  echo "Run: python3 tools/generate_driver_hal_dependency_map.py"
  diff -u docs/planning/DRIVER_HAL_DEPENDENCY_MAP.md "$GEN_MD" || true
  exit 1
fi

python3 - <<'PY'
import json
from pathlib import Path

payload = json.loads(Path("docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json").read_text(encoding="utf-8"))
summary = payload.get("summary") or {}
modules = payload.get("modules") or []

if int(summary.get("modules_total", 0)) != len(modules):
    raise SystemExit("FAIL: modules_total does not match modules length")
if int(summary.get("unique_dependencies", 0)) < int(summary.get("hal_dependencies", 0)):
    raise SystemExit("FAIL: unique_dependencies smaller than hal_dependencies")

print("PASS: driver HAL dependency map drift smoke checks passed")
PY
