#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_configurator_api_smoke"
rm -rf "$OUT"
mkdir -p "$OUT"

python3 - <<'PY' >"$OUT/server.log" 2>&1 &
from tools import basaltos_config_server as srv
srv.app.run(debug=False, use_reloader=False, host="127.0.0.1", port=5000)
PY
SERVER_PID=$!

cleanup() {
  kill "$SERVER_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Wait for server readiness
python3 - <<'PY'
import time, urllib.request, sys
url='http://127.0.0.1:5000/'
for _ in range(60):
    try:
        with urllib.request.urlopen(url, timeout=1) as r:
            if r.status == 200:
                print('server ready')
                sys.exit(0)
    except Exception:
        time.sleep(0.5)
print('server failed to start')
sys.exit(1)
PY

curl --fail-with-body -sS http://127.0.0.1:5000/api/platforms > "$OUT/platforms.json"
curl --fail-with-body -sS http://127.0.0.1:5000/api/boards/esp32 > "$OUT/boards_esp32.json"
curl --fail-with-body -sS "http://127.0.0.1:5000/api/drivers?platform=esp32" > "$OUT/drivers_esp32.json"
curl --fail-with-body -sS http://127.0.0.1:5000/api/board/esp32-c3-supermini > "$OUT/board_esp32c3.json"
curl --fail-with-body -sS http://127.0.0.1:5000/api/sync/export-preview > "$OUT/sync_export_preview.json"
curl --fail-with-body -sS http://127.0.0.1:5000/api/board-taxonomy > "$OUT/board_taxonomy.json"
curl --fail-with-body -sS "http://127.0.0.1:5000/api/board-taxonomy?platform=esp32&architecture=risc-v" > "$OUT/board_taxonomy_filtered.json"

cat > "$OUT/generate_payload.json" <<'JSON'
{
  "platform": "esp32",
  "board_id": "esp32-c3-supermini",
  "enabled_drivers": ["uart"],
  "custom_pins": {},
  "applets": [],
  "market_apps": []
}
JSON

curl --fail-with-body -sS -X POST http://127.0.0.1:5000/api/generate \
  -H 'Content-Type: application/json' \
  -d @"$OUT/generate_payload.json" > "$OUT/generate_response.json"

curl --fail-with-body -sS -X POST http://127.0.0.1:5000/api/preview/basalt_config_h \
  -H 'Content-Type: application/json' \
  -d @"$OUT/generate_payload.json" > "$OUT/preview_response.json"

cat > "$OUT/conflict_payload.json" <<'JSON'
{
  "platform": "esp32",
  "board_id": "esp32-c3-supermini",
  "enabled_drivers": ["uart", "i2c"],
  "custom_pins": {
    "i2c_sda": 4,
    "i2c_scl": 4
  },
  "applets": [],
  "market_apps": []
}
JSON

curl -sS -w "\nHTTP_CODE:%{http_code}\n" -X POST http://127.0.0.1:5000/api/generate \
  -H 'Content-Type: application/json' \
  -d @"$OUT/conflict_payload.json" > "$OUT/conflict_response.txt"

python3 - <<'PY'
import json
from pathlib import Path
out=Path('tmp/test_configurator_api_smoke')

for fn in ['platforms.json','boards_esp32.json','drivers_esp32.json','board_esp32c3.json','sync_export_preview.json','board_taxonomy.json','board_taxonomy_filtered.json','generate_response.json','preview_response.json']:
    p=out/fn
    d=json.load(open(p,'r',encoding='utf-8'))
    if not d:
        raise SystemExit(f'empty response: {fn}')

gen=json.load(open(out/'generate_response.json','r',encoding='utf-8'))
if not gen.get('success'):
    raise SystemExit('generate API did not return success=true')

prev=json.load(open(out/'preview_response.json','r',encoding='utf-8'))
if not prev.get('success'):
    raise SystemExit('preview API did not return success=true')

sync=json.load(open(out/'sync_export_preview.json','r',encoding='utf-8'))
if not sync.get('success'):
    raise SystemExit('sync export preview API did not return success=true')
if sync.get('schema_version') != '1.0.0':
    raise SystemExit('sync export preview schema mismatch')
if 'envelope' not in sync:
    raise SystemExit('sync export preview missing envelope')

tax=json.load(open(out/'board_taxonomy.json','r',encoding='utf-8'))
if not tax.get('success'):
    raise SystemExit('board taxonomy API did not return success=true')
if int((tax.get('summary') or {}).get('total_boards', 0)) <= 0:
    raise SystemExit('board taxonomy summary total_boards must be > 0')
if not isinstance(tax.get('boards'), list) or len(tax.get('boards')) == 0:
    raise SystemExit('board taxonomy boards list is empty')

tax_f=json.load(open(out/'board_taxonomy_filtered.json','r',encoding='utf-8'))
if not tax_f.get('success'):
    raise SystemExit('filtered board taxonomy API did not return success=true')
if tax_f.get('filters', {}).get('platform') != 'esp32':
    raise SystemExit('filtered taxonomy did not echo platform filter')
if tax_f.get('filters', {}).get('architecture') != 'risc-v':
    raise SystemExit('filtered taxonomy did not echo architecture filter')

conf=(out/'conflict_response.txt').read_text(encoding='utf-8')
if 'HTTP_CODE:400' not in conf:
    raise SystemExit('expected conflict call HTTP 400')
if 'Pin validation failed' not in conf:
    raise SystemExit('expected pin validation message not found')

print('PASS: configurator API smoke checks')
PY
