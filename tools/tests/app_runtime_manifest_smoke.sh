#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path
import tempfile

from tools.app_validation import validate_app_dir

with tempfile.TemporaryDirectory() as td:
    root = Path(td)

    # python default
    app_py = root / "app_py"
    app_py.mkdir()
    (app_py / "app.toml").write_text('name="PyApp"\nversion="0.1.0"\nruntime="python"\n', encoding="utf-8")
    (app_py / "main.py").write_text('print("ok")\n', encoding="utf-8")
    meta_py = validate_app_dir(app_py)
    assert meta_py["runtime"] == "python"
    assert meta_py["entry"] == "main.py"

    # lua default entry
    app_lua = root / "app_lua"
    app_lua.mkdir()
    (app_lua / "app.toml").write_text('name="LuaApp"\nversion="0.1.0"\nruntime="lua"\n', encoding="utf-8")
    (app_lua / "main.lua").write_text('print("ok")\n', encoding="utf-8")
    meta_lua = validate_app_dir(app_lua)
    assert meta_lua["runtime"] == "lua"
    assert meta_lua["entry"] == "main.lua"

    # no manifest fallback prefers py then lua
    app_nomani = root / "app_nomani"
    app_nomani.mkdir()
    (app_nomani / "main.lua").write_text('print("ok")\n', encoding="utf-8")
    meta_nomani = validate_app_dir(app_nomani)
    assert meta_nomani["entry"] == "main.lua"
    assert meta_nomani["runtime"] == "python"

    # unsupported runtime must fail
    app_bad = root / "app_bad"
    app_bad.mkdir()
    (app_bad / "app.toml").write_text('name="Bad"\nversion="0.1.0"\nruntime="ruby"\nentry="main.rb"\n', encoding="utf-8")
    (app_bad / "main.rb").write_text('puts("x")\n', encoding="utf-8")
    try:
        validate_app_dir(app_bad)
    except ValueError as e:
        assert "unsupported runtime" in str(e)
    else:
        raise SystemExit("expected unsupported runtime validation failure")

print("PASS: app runtime manifest smoke checks")
PY
