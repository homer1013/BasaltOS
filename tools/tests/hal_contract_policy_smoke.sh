#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

POLICY_JSON="docs/planning/HAL_CONTRACT_POLICY.json"
MATRIX_JSON="docs/planning/HAL_ADAPTER_MATRIX.json"

[[ -f "$POLICY_JSON" ]] || { echo "FAIL: missing $POLICY_JSON"; exit 1; }
[[ -f "$MATRIX_JSON" ]] || { echo "FAIL: missing $MATRIX_JSON"; exit 1; }

python3 - <<'PY'
import json
from pathlib import Path

policy = json.loads(Path("docs/planning/HAL_CONTRACT_POLICY.json").read_text(encoding="utf-8"))
matrix = json.loads(Path("docs/planning/HAL_ADAPTER_MATRIX.json").read_text(encoding="utf-8"))

allowed = set(policy.get("allowed_status_values") or [])
ack_partial = set(policy.get("acknowledged_partial_ports") or [])
ack_missing = set(policy.get("acknowledged_missing_ports") or [])
taxonomy = set((policy.get("status_taxonomy") or {}).keys())

if not allowed:
    raise SystemExit("FAIL: HAL policy has no allowed_status_values")
if taxonomy != allowed:
    raise SystemExit(
        f"FAIL: status_taxonomy keys {sorted(taxonomy)} must match allowed_status_values {sorted(allowed)}"
    )

rows = matrix.get("rows") or []
if not rows:
    raise SystemExit("FAIL: HAL adapter matrix has no rows")

unexpected_status = []
unacked_partial = []
unacked_missing = []
count_partial = 0
count_missing = 0

for row in rows:
    port = str(row.get("port") or "")
    status = str(row.get("status") or "")
    present = row.get("present") or []
    missing = row.get("missing") or []
    present_count = int(row.get("present_count") or 0)
    missing_count = int(row.get("missing_count") or 0)

    if status not in allowed:
        unexpected_status.append((port, status))

    if present_count != len(present):
        raise SystemExit(f"FAIL: {port} present_count mismatch ({present_count} vs {len(present)})")
    if missing_count != len(missing):
        raise SystemExit(f"FAIL: {port} missing_count mismatch ({missing_count} vs {len(missing)})")

    if status == "partial":
        count_partial += 1
        if port not in ack_partial:
            unacked_partial.append(port)
    elif status == "missing":
        count_missing += 1
        if port not in ack_missing:
            unacked_missing.append(port)

if unexpected_status:
    raise SystemExit(f"FAIL: unexpected HAL adapter statuses: {unexpected_status}")
if unacked_partial:
    raise SystemExit(f"FAIL: unacknowledged partial HAL ports: {sorted(unacked_partial)}")
if unacked_missing:
    raise SystemExit(f"FAIL: unacknowledged missing HAL ports: {sorted(unacked_missing)}")

summary = matrix.get("summary") or {}
status_counts = summary.get("status_counts") or {}
if int(status_counts.get("partial", 0)) != count_partial:
    raise SystemExit("FAIL: summary partial count mismatch")
if int(status_counts.get("missing", 0)) != count_missing:
    raise SystemExit("FAIL: summary missing count mismatch")

print("PASS: HAL contract policy smoke checks passed")
PY
