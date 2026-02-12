#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/mcp2544fd_posture_report_drift"
rm -rf "$TMP"
mkdir -p "$TMP"

GEN_JSON="$TMP/MCP2544FD_CAPABILITY_POSTURE.generated.json"
GEN_MD="$TMP/MCP2544FD_CAPABILITY_POSTURE.generated.md"

python3 tools/generate_mcp2544fd_posture_report.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD" >/dev/null

if ! diff -u docs/planning/MCP2544FD_CAPABILITY_POSTURE.json "$GEN_JSON" >/dev/null; then
  echo "FAIL: docs/planning/MCP2544FD_CAPABILITY_POSTURE.json is out of date"
  echo "Run: python3 tools/generate_mcp2544fd_posture_report.py"
  diff -u docs/planning/MCP2544FD_CAPABILITY_POSTURE.json "$GEN_JSON" || true
  exit 1
fi

if ! diff -u docs/planning/MCP2544FD_CAPABILITY_POSTURE.md "$GEN_MD" >/dev/null; then
  echo "FAIL: docs/planning/MCP2544FD_CAPABILITY_POSTURE.md is out of date"
  echo "Run: python3 tools/generate_mcp2544fd_posture_report.py"
  diff -u docs/planning/MCP2544FD_CAPABILITY_POSTURE.md "$GEN_MD" || true
  exit 1
fi

python3 - <<'PY'
import json
from pathlib import Path
p = Path("docs/planning/MCP2544FD_CAPABILITY_POSTURE.json")
d = json.loads(p.read_text(encoding="utf-8"))
actionable = int(((d.get("summary") or {}).get("actionable_total")) or 0)
if actionable != 0:
    raise SystemExit(
        f"FAIL: MCP2544FD posture actionable_total={actionable}; review docs/planning/MCP2544FD_CAPABILITY_POSTURE.md"
    )
print("PASS: MCP2544FD posture actionable_total is zero")
PY

echo "PASS: mcp2544fd posture report drift smoke checks passed"
