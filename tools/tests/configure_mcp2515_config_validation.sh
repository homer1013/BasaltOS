#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/tmp/mcp2515_config_validation"
TEMPLATES="$ROOT/config/templates/project_templates.json"
BACKUP="$OUT/project_templates.backup.json"

rm -rf "$OUT"
mkdir -p "$OUT"
cp "$TEMPLATES" "$BACKUP"
restore() {
  cp "$BACKUP" "$TEMPLATES"
}
trap restore EXIT

python3 - <<'PY' "$TEMPLATES"
import json,sys
p=sys.argv[1]
d=json.load(open(p,'r',encoding='utf-8'))
t=d.get('templates',[])

t.append({
  "id":"mcp2515_invalid_cfg",
  "name":"MCP2515 Invalid Config Test",
  "description":"Intentional invalid config values for validation test",
  "platforms":["esp32"],
  "enabled_drivers":["uart","spi","mcp2515"],
  "applets":[],
  "driver_config":{
    "mcp2515":{
      "bitrate":12345,
      "oscillator_hz":123
    }
  },
  "tags":["test"]
})

json.dump(d,open(p,'w',encoding='utf-8'),indent=2)
print('template injected')
PY

set +e
python3 "$ROOT/tools/configure.py" \
  --platform esp32 \
  --board esp32-c3-supermini \
  --template mcp2515_invalid_cfg \
  --outdir "$OUT/generated" >"$OUT/run.log" 2>&1
rc=$?
set -e

if [[ $rc -eq 0 ]]; then
  echo "Expected configure failure for invalid mcp2515 config, but command succeeded"
  cat "$OUT/run.log"
  exit 1
fi

grep -q "driver_config.mcp2515.bitrate: invalid value" "$OUT/run.log"
grep -q "driver_config.mcp2515.oscillator_hz: invalid value" "$OUT/run.log"

echo "PASS: mcp2515 config validation"
