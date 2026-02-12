#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 tools/generate_generation_parity_baseline.py >/dev/null

python3 - <<'PY'
import json
from pathlib import Path

p = Path("docs/planning/GENERATION_PARITY_BASELINE.json")
d = json.loads(p.read_text(encoding="utf-8"))
summary = d.get("summary") or {}
count = int(summary.get("with_defaults_mode_fail_count", 0))
if count != 0:
    raise SystemExit(
        f"FAIL: with_defaults_mode_fail_count={count} (expected 0). "
        "Run: python3 tools/generate_generation_parity_baseline.py and fix failing board defaults parity."
    )
print("PASS: generation parity defaults gate smoke checks passed")
PY

