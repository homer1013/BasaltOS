#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_configurator_api_smoke"
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

# Wait for server readiness
python3 - <<'PY'
import time, urllib.request, sys
import os
port = os.environ["BASALT_TEST_PORT"]
url=f'http://127.0.0.1:{port}/'
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

curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/platforms" > "$OUT/platforms.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/boards/esp32" > "$OUT/boards_esp32.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/boards/esp32?id=esp32-c3-supermini" > "$OUT/boards_esp32_filtered.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/drivers?platform=esp32" > "$OUT/drivers_esp32.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board/esp32-c3-supermini" > "$OUT/board_esp32c3.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/sync/export-preview" > "$OUT/sync_export_preview.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy" > "$OUT/board_taxonomy.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy?platform=esp32&architecture=risc-v" > "$OUT/board_taxonomy_filtered.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy/options" > "$OUT/board_taxonomy_options.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy/options?platform=esp32" > "$OUT/board_taxonomy_options_filtered.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy/meta" > "$OUT/board_taxonomy_meta.json"
curl --fail-with-body -sS "http://127.0.0.1:${PORT}/api/board-taxonomy/lookup/esp32-c3-supermini" > "$OUT/board_taxonomy_lookup.json"
curl -sS -w "\nHTTP_CODE:%{http_code}\n" "http://127.0.0.1:${PORT}/api/board-taxonomy/lookup/not-a-real-board" > "$OUT/board_taxonomy_lookup_missing.txt"

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

curl --fail-with-body -sS -X POST "http://127.0.0.1:${PORT}/api/generate" \
  -H 'Content-Type: application/json' \
  -d @"$OUT/generate_payload.json" > "$OUT/generate_response.json"

curl --fail-with-body -sS -X POST "http://127.0.0.1:${PORT}/api/preview/basalt_config_h" \
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

curl -sS -w "\nHTTP_CODE:%{http_code}\n" -X POST "http://127.0.0.1:${PORT}/api/generate" \
  -H 'Content-Type: application/json' \
  -d @"$OUT/conflict_payload.json" > "$OUT/conflict_response.txt"

python3 - <<'PY'
import json
from pathlib import Path
out=Path('tmp/test_configurator_api_smoke')

for fn in ['platforms.json','boards_esp32.json','boards_esp32_filtered.json','drivers_esp32.json','board_esp32c3.json','sync_export_preview.json','board_taxonomy.json','board_taxonomy_filtered.json','board_taxonomy_options.json','board_taxonomy_options_filtered.json','board_taxonomy_meta.json','board_taxonomy_lookup.json','generate_response.json','preview_response.json']:
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

boards_f=json.load(open(out/'boards_esp32_filtered.json','r',encoding='utf-8'))
if not isinstance(boards_f, list):
    raise SystemExit('filtered /api/boards must return a list')
if len(boards_f) != 1:
    raise SystemExit('filtered /api/boards by id should return exactly one board')
if str((boards_f[0] or {}).get('id')) != 'esp32-c3-supermini':
    raise SystemExit('filtered /api/boards returned unexpected board id')

tax_f=json.load(open(out/'board_taxonomy_filtered.json','r',encoding='utf-8'))
if not tax_f.get('success'):
    raise SystemExit('filtered board taxonomy API did not return success=true')
if tax_f.get('filters', {}).get('platform') != 'esp32':
    raise SystemExit('filtered taxonomy did not echo platform filter')
if tax_f.get('filters', {}).get('architecture') != 'risc-v':
    raise SystemExit('filtered taxonomy did not echo architecture filter')

opt=json.load(open(out/'board_taxonomy_options.json','r',encoding='utf-8'))
if not opt.get('success'):
    raise SystemExit('board taxonomy options API did not return success=true')
if not isinstance((opt.get('options') or {}).get('platform'), list):
    raise SystemExit('taxonomy options missing platform list')
if len((opt.get('options') or {}).get('platform') or []) == 0:
    raise SystemExit('taxonomy options platform list should not be empty')

opt_f=json.load(open(out/'board_taxonomy_options_filtered.json','r',encoding='utf-8'))
if not opt_f.get('success'):
    raise SystemExit('filtered board taxonomy options API did not return success=true')
if opt_f.get('filters', {}).get('platform') != 'esp32':
    raise SystemExit('filtered taxonomy options did not echo platform filter')
if len((opt_f.get('options') or {}).get('platform') or []) > 1:
    raise SystemExit('platform-filtered taxonomy options should include at most one platform')

meta=json.load(open(out/'board_taxonomy_meta.json','r',encoding='utf-8'))
if not meta.get('success'):
    raise SystemExit('board taxonomy meta API did not return success=true')
sha=(meta.get('meta') or {}).get('sha256','')
if len(sha) != 64:
    raise SystemExit('taxonomy meta sha256 should be 64 chars')
if int((meta.get('summary') or {}).get('total_boards', 0)) != int((tax.get('summary') or {}).get('total_boards', -1)):
    raise SystemExit('taxonomy meta and taxonomy data total_boards mismatch')

lookup=json.load(open(out/'board_taxonomy_lookup.json','r',encoding='utf-8'))
if not lookup.get('success'):
    raise SystemExit('taxonomy lookup API did not return success=true')
if (lookup.get('board') or {}).get('id') != 'esp32-c3-supermini':
    raise SystemExit('taxonomy lookup returned unexpected board id')

lookup_missing=(out/'board_taxonomy_lookup_missing.txt').read_text(encoding='utf-8')
if 'HTTP_CODE:404' not in lookup_missing:
    raise SystemExit('taxonomy lookup missing-board should return HTTP 404')

conf=(out/'conflict_response.txt').read_text(encoding='utf-8')
if 'HTTP_CODE:400' not in conf:
    raise SystemExit('expected conflict call HTTP 400')
if 'Pin validation failed' not in conf:
    raise SystemExit('expected pin validation message not found')

print('PASS: configurator API smoke checks')
PY
