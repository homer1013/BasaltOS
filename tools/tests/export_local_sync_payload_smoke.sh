#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/test_export_local_sync_payload"
REPO="$TMP/repo"
LOCAL="$REPO/.basaltos-local"
OUT="$TMP/out.json"

rm -rf "$TMP"
mkdir -p "$REPO/config/generated"
mkdir -p "$LOCAL/user/configs" "$LOCAL/user/boards" "$LOCAL/user/presets"

cat > "$REPO/config/generated/basalt_config.json" <<'JSON'
{ "platform": "esp32", "board": "esp32-c3-supermini" }
JSON

cat > "$REPO/config/generated/basalt.features.json" <<'JSON'
{ "enabled_drivers": ["uart", "gpio"] }
JSON

cat > "$LOCAL/user/configs/starter.json" <<'JSON'
{ "name": "starter", "drivers": ["uart"] }
JSON

cat > "$LOCAL/user/boards/inventory.json" <<'JSON'
{ "owned": ["esp32-c3-supermini"] }
JSON

cat > "$LOCAL/user/presets/default.json" <<'JSON'
{ "preset": "default" }
JSON

python3 tools/export_local_sync_payload.py \
  --repo-root "$REPO" \
  --local-data-root ".basaltos-local" \
  --app-version v0.1.0 \
  --out "$OUT" >/tmp/export_local_sync_payload.out

python3 - "$OUT" <<'PY'
import json,sys,re
p=sys.argv[1]
d=json.load(open(p,'r',encoding='utf-8'))
assert d["schema_version"]=="1.0.0"
assert d["source"]["kind"]=="basaltos_main_local"
assert d["source"]["app_version"]=="v0.1.0"
assert re.match(r"^dev_[0-9a-f]{16}$", d["source"]["device_id"])
items=d.get("items",[])
assert len(items)>=4, len(items)
types={i["item_type"] for i in items}
assert "config_snapshot" in types
assert "board_inventory" in types
assert "user_preset" in types
for i in items:
    assert i["content_hash"].startswith("sha256:")
print("ok")
PY

grep -q "\[sync-export\] wrote:" /tmp/export_local_sync_payload.out
grep -q "\[sync-export\] items:" /tmp/export_local_sync_payload.out

echo "PASS: local sync payload export smoke"
