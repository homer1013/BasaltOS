#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import re

prims = []
for hdr in sorted(Path("basalt_hal/include/hal").glob("hal_*.h")):
    m = re.match(r"hal_([a-z0-9_]+)\.h$", hdr.name)
    if not m:
        continue
    p = m.group(1)
    if p == "types":
        continue
    prims.append(p)

ports = ["esp32", "esp32c3", "esp32c6", "esp32s3"]
issues = []
for port in ports:
    pdir = Path("basalt_hal/ports") / port
    for prim in prims:
        src = pdir / f"hal_{prim}.c"
        if not src.exists():
            continue
        text = src.read_text(encoding="utf-8")
        if text.lstrip().startswith("#pragma once"):
            issues.append(f"{src}: appears to be a header stub, not a C impl")
        if f"hal_{prim}_" not in text:
            issues.append(f"{src}: missing hal_{prim}_ symbol definitions")

if issues:
    raise SystemExit("FAIL: HAL impl sanity violations:\n" + "\n".join(issues))

print("PASS: HAL impl sanity smoke checks")
PY
