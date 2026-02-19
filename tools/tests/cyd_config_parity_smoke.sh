#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_cyd_config_parity"
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
import time, urllib.request, sys, os
url = f'http://127.0.0.1:{os.environ["BASALT_TEST_PORT"]}/'
for _ in range(60):
    try:
        with urllib.request.urlopen(url, timeout=1) as r:
            if r.status == 200:
                print("server ready")
                sys.exit(0)
    except Exception:
        time.sleep(0.5)
print("server failed to start")
sys.exit(1)
PY

cat > "$OUT/payload.json" <<'JSON'
{
  "platform": "esp32",
  "board_id": "cyd",
  "enabled_drivers": ["gpio","uart","spi","i2c","fs_spiffs","fs_sd","tft","shell_full"],
  "applets": ["tft_hello","system_info","sd_probe"],
  "market_apps": [],
  "driver_config": {},
  "custom_pins": {}
}
JSON

curl --fail-with-body -sS -X POST "http://127.0.0.1:${PORT}/api/generate" \
  -H 'Content-Type: application/json' \
  -d @"$OUT/payload.json" > "$OUT/generate_response.json"

python3 tools/configure.py \
  --platform esp32 \
  --board cyd_3248s035r \
  --enable-drivers gpio,uart,spi,i2c,fs_spiffs,fs_sd,tft,shell_full \
  --applets tft_hello,system_info,sd_probe \
  --outdir "$OUT/cli"

python3 - <<'PY'
import difflib
import json
import re
import sys
from pathlib import Path

root = Path("tmp/test_cyd_config_parity")
pairs = [
    (Path("config/generated/basalt_config.h"), root / "cli/basalt_config.h"),
    (Path("config/generated/sdkconfig.defaults"), root / "cli/sdkconfig.defaults"),
    (Path("config/generated/applets.json"), root / "cli/applets.json"),
    (Path("config/generated/basalt.features.json"), root / "cli/basalt.features.json"),
    (Path("config/generated/basalt_config.json"), root / "cli/basalt_config.json"),
]

def normalize_json_lines(lines):
    out = []
    for ln in lines:
        out.append(re.sub(r'"generated_utc"\s*:\s*"[^"]+"', '"generated_utc":"<ts>"', ln))
    return out

for a, b in pairs:
    if not a.exists() or not b.exists():
        raise SystemExit(f"missing file(s): {a} exists={a.exists()} | {b} exists={b.exists()}")
    la = a.read_text(encoding="utf-8", errors="replace").splitlines()
    lb = b.read_text(encoding="utf-8", errors="replace").splitlines()
    if a.suffix == ".json":
        la = normalize_json_lines(la)
        lb = normalize_json_lines(lb)
    if la != lb:
        diff = "\n".join(difflib.unified_diff(la, lb, fromfile=str(a), tofile=str(b), n=2))
        raise SystemExit(f"parity mismatch for {a.name}\n{diff}")

print("PASS: CYD CLI/API parity smoke")
PY

