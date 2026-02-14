#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import tempfile

from tools.app_validation import validate_app_dir, validate_zip_package

root = Path(".").resolve()

apps = [
    root / "apps" / "lua_hello.app",
    root / "apps" / "lua_blink.app",
    root / "apps" / "demo.app",
]

for app in apps:
    meta = validate_app_dir(app)
    if app.name.startswith("lua_"):
        assert meta["runtime"] == "lua", f"{app}: expected runtime lua"
        assert meta["entry"] == "main.lua", f"{app}: expected entry main.lua"
    else:
        assert meta["runtime"] == "python", f"{app}: expected runtime python"
        assert meta["entry"] == "main.py", f"{app}: expected entry main.py"

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    for app in apps:
        import subprocess
        out = td / f"{app.stem}.zip"
        subprocess.run(
            ["python3", "tools/pack_app.py", str(app), str(out)],
            check=True,
        )
        payload = out.read_bytes()
        zm = validate_zip_package(payload)["meta"]
        dm = validate_app_dir(app)
        assert zm["runtime"] == dm["runtime"], f"{app}: runtime mismatch after pack"
        assert zm["entry"] == dm["entry"], f"{app}: entry mismatch after pack"

print("PASS: lua examples parity smoke checks")
PY
