#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_board_taxonomy_api_contract"
rm -rf "$OUT"
mkdir -p "$OUT"

PORT="${BASALT_TEST_PORT:-}"
if [[ -z "$PORT" ]]; then
  PORT="$(python3 - <<'PY'
import socket
s = socket.socket()
s.bind(("127.0.0.1", 0))
print(s.getsockname()[1])
s.close()
PY
)"
fi
export BASALT_TEST_PORT="$PORT"

python3 - <<'PY' >"$OUT/server.log" 2>&1 &
from tools import basaltos_config_server as srv
import os
port = int(os.environ["BASALT_TEST_PORT"])
srv.app.run(debug=False, use_reloader=False, host="127.0.0.1", port=port)
PY
SERVER_PID=$!

cleanup() {
  kill "$SERVER_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

python3 - <<'PY'
import os, sys, time, urllib.request
port = os.environ["BASALT_TEST_PORT"]
url = f"http://127.0.0.1:{port}/"
for _ in range(60):
    try:
        with urllib.request.urlopen(url, timeout=1) as r:
            if r.status == 200:
                sys.exit(0)
    except Exception:
        time.sleep(0.5)
print("server failed to start")
sys.exit(1)
PY

curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy?platform=esp32&architecture=risc-v" > "$OUT/taxonomy.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/boards/esp32/options?manufacturer=Espressif&architecture=RISC-V&q=c3" > "$OUT/options.json"

python3 - <<'PY'
import json
from pathlib import Path

out = Path("tmp/test_board_taxonomy_api_contract")
tax = json.loads((out / "taxonomy.json").read_text(encoding="utf-8"))
opt = json.loads((out / "options.json").read_text(encoding="utf-8"))

for obj_name, obj in [("taxonomy", tax), ("options", opt)]:
    if not obj.get("success"):
        raise SystemExit(f"FAIL: {obj_name} response success=false")
    if "filters" not in obj or not isinstance(obj.get("filters"), dict):
        raise SystemExit(f"FAIL: {obj_name} response missing filters object")
    if "summary" not in obj or not isinstance(obj.get("summary"), dict):
        raise SystemExit(f"FAIL: {obj_name} response missing summary object")
    if "counts" not in obj or not isinstance(obj.get("counts"), dict):
        raise SystemExit(f"FAIL: {obj_name} response missing counts object")

if tax["filters"].get("platform") != "esp32":
    raise SystemExit("FAIL: taxonomy filters.platform mismatch")
if tax["filters"].get("architecture") != "risc-v":
    raise SystemExit("FAIL: taxonomy filters.architecture mismatch")
if not isinstance(tax.get("boards"), list):
    raise SystemExit("FAIL: taxonomy boards should be a list")
if int((tax.get("summary") or {}).get("total_boards", 0)) < 1:
    raise SystemExit("FAIL: taxonomy total_boards should be >= 1 for esp32+risc-v")

if opt["filters"].get("platform") != "esp32":
    raise SystemExit("FAIL: options filters.platform mismatch")
if "options" not in opt or not isinstance(opt.get("options"), dict):
    raise SystemExit("FAIL: options response missing options object")

for key in ("manufacturer", "architecture", "family", "silicon"):
    if key not in (opt.get("counts") or {}):
        raise SystemExit(f"FAIL: options counts missing '{key}'")
    if key not in (opt.get("options") or {}):
        raise SystemExit(f"FAIL: options options missing '{key}'")

print("PASS: board taxonomy API contract smoke checks passed")
PY
