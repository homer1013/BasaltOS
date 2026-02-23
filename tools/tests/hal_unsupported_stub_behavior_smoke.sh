#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

issues = []

for src in sorted(Path("basalt_hal/ports").glob("*/hal_*.c")):
    text = src.read_text(encoding="utf-8")
    if "BASALT_HAL_UNSUPPORTED_STUB" not in text:
        continue

    if text.lstrip().startswith("#pragma once"):
        issues.append(f"{src}: unsupported stub is header-like (#pragma once)")
        continue

    if "return -ENOTSUP;" not in text:
        issues.append(f"{src}: unsupported stub missing return -ENOTSUP;")

    if " return 0;" in text or "return 0;" in text:
        issues.append(f"{src}: unsupported stub must not return success")

if issues:
    raise SystemExit("FAIL: HAL unsupported stub behavior violations:\n" + "\n".join(issues))

print("PASS: HAL unsupported stub behavior smoke checks")
PY
