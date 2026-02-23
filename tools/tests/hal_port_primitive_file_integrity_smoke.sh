#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import re

ports = sorted([p.name for p in Path("basalt_hal/ports").iterdir() if p.is_dir()])
prims = []
for hdr in sorted(Path("basalt_hal/include/hal").glob("hal_*.h")):
    m = re.match(r"hal_([a-z0-9_]+)\.h$", hdr.name)
    if not m:
        continue
    p = m.group(1)
    if p == "types":
        continue
    prims.append(p)

issues = []
for port in ports:
    pdir = Path("basalt_hal/ports") / port
    for prim in prims:
        src = pdir / f"hal_{prim}.c"
        if not src.exists():
            continue
        text = src.read_text(encoding="utf-8")
        if text.lstrip().startswith("#pragma once"):
            issues.append(f"{src}: C source is a header-style stub (#pragma once)")
        if "BASALT_HAL_UNSUPPORTED_STUB" in text and f"hal_{prim}_" not in text:
            issues.append(f"{src}: unsupported stub marker present but missing hal_{prim}_ symbols")

if issues:
    raise SystemExit("FAIL: HAL primitive file integrity violations:\n" + "\n".join(issues))

print("PASS: HAL primitive file integrity smoke checks")
PY
