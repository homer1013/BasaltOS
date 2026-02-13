#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PKG="tools/e2e/package.json"
README="tools/e2e/README.md"

[[ -f "$PKG" ]] || { echo "FAIL: missing $PKG"; exit 1; }
[[ -f "$README" ]] || { echo "FAIL: missing $README"; exit 1; }

python3 - <<'PY'
import json
from pathlib import Path

pkg = json.loads(Path("tools/e2e/package.json").read_text(encoding="utf-8"))

deps = pkg.get("dependencies") or {}
scripts = pkg.get("scripts") or {}

if "playwright" not in deps:
    raise SystemExit("FAIL: tools/e2e/package.json missing playwright dependency")
if not str(deps.get("playwright", "")).strip():
    raise SystemExit("FAIL: playwright dependency version is empty")

required_scripts = {"e2e:smoke", "e2e:deep", "e2e:local", "e2e:all", "e2e:install-browsers"}
missing = sorted([s for s in required_scripts if s not in scripts])
if missing:
    raise SystemExit(f"FAIL: missing e2e scripts in package.json: {', '.join(missing)}")

print("PASS: e2e package contract OK")
PY

if command -v rg >/dev/null 2>&1; then
  rg -q "npm --prefix tools/e2e install" "$README" || { echo "FAIL: README missing install command"; exit 1; }
  rg -q "e2e:smoke" "$README" || { echo "FAIL: README missing smoke command"; exit 1; }
  rg -q "e2e:deep" "$README" || { echo "FAIL: README missing deep command"; exit 1; }
else
  grep -q "npm --prefix tools/e2e install" "$README" || { echo "FAIL: README missing install command"; exit 1; }
  grep -q "e2e:smoke" "$README" || { echo "FAIL: README missing smoke command"; exit 1; }
  grep -q "e2e:deep" "$README" || { echo "FAIL: README missing deep command"; exit 1; }
fi

echo "PASS: e2e playwright harness smoke"
