#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/configure_quickstart_profile_smoke"
rm -rf "$TMP"
mkdir -p "$TMP"

python3 tools/configure.py --list-profiles >"$TMP/profiles.txt"
grep -q "esp32_c6_quickstart" "$TMP/profiles.txt"

auto_out="$TMP/quickstart_profile"
python3 tools/configure.py --quickstart esp32_c6_quickstart --outdir "$auto_out" >/dev/null

test -f "$auto_out/basalt_config.json"
python3 - <<'PY'
import json
from pathlib import Path
cfg = json.loads(Path('tmp/configure_quickstart_profile_smoke/quickstart_profile/basalt_config.json').read_text(encoding='utf-8'))
assert cfg['platform'] == 'esp32', cfg
assert cfg['board_dir'] == 'esp32-c6', cfg
assert 'uart' in cfg['enabled_drivers'], cfg
PY

saved="$TMP/saved_profile"
python3 tools/configure.py --quickstart esp32_c6_quickstart --outdir "$TMP/savegen" --profile-save "$saved" >/dev/null
test -f "$saved.json"

python3 tools/configure.py --profile-load "$saved.json" --outdir "$TMP/loadgen" >/dev/null
test -f "$TMP/loadgen/basalt_config.json"

echo "PASS: configure quickstart/profile smoke checks"
