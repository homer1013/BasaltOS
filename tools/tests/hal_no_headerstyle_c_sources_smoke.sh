#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

bad = []
for src in sorted(Path("basalt_hal/ports").glob("*/hal_*.c")):
    text = src.read_text(encoding="utf-8")
    if text.lstrip().startswith("#pragma once"):
        bad.append(str(src))

if bad:
    raise SystemExit("FAIL: header-style C sources detected:\n" + "\n".join(bad))

print("PASS: HAL C-source style smoke checks")
PY
