#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/test_sync_payload_tooling"
REPO="$TMP/repo"
LOCAL="$REPO/.basaltos-local"
rm -rf "$TMP"
mkdir -p "$REPO/config/generated" "$LOCAL/user/configs" "$LOCAL/user/boards" "$LOCAL/user/presets"

cat > "$REPO/config/generated/basalt_config.json" <<'JSON'
{ "platform": "esp32", "board": "esp32-c3-supermini" }
JSON

cat > "$LOCAL/user/configs/starter.json" <<'JSON'
{ "name": "starter", "version": 1 }
JSON

python3 tools/export_local_sync_payload.py \
  --repo-root "$REPO" \
  --local-data-root ".basaltos-local" \
  --out "$TMP/local_payload.json" >/tmp/sync_payload_tooling_export.out

python3 - "$TMP/local_payload.json" "$TMP/remote_payload.json" <<'PY'
import json,sys,copy
local=json.load(open(sys.argv[1],'r',encoding='utf-8'))
remote=copy.deepcopy(local)
for item in remote.get('items',[]):
    if item.get('item_id')=='config_local_starter.json':
        item['content']={'path':'starter.json','data':{'name':'starter','version':2}}
        item['content_hash']='sha256:changed'
remote['items'].append({
    'item_type':'user_preset',
    'item_id':'preset_cloud_added',
    'updated_utc':'2026-02-12T00:00:00Z',
    'content_hash':'sha256:new',
    'content':{'path':'cloud.json','data':{'preset':'cloud'}}
})
json.dump(remote, open(sys.argv[2],'w',encoding='utf-8'), indent=2)
PY

python3 tools/diff_local_sync_payload.py \
  --local "$TMP/local_payload.json" \
  --remote "$TMP/remote_payload.json" \
  --json-out "$TMP/diff.json" >/tmp/sync_payload_tooling_diff.out

python3 - "$TMP/diff.json" <<'PY'
import json,sys
d=json.load(open(sys.argv[1],'r',encoding='utf-8'))
s=d['summary']
assert s['changed'] >= 1, s
assert s['remote_only'] >= 1, s
print('ok')
PY

mkdir -p "$LOCAL/user/configs"
cat > "$LOCAL/user/configs/config_local_starter.json.json" <<'JSON'
{ "name": "starter", "version": 1 }
JSON

python3 tools/import_local_sync_payload.py \
  --repo-root "$REPO" \
  --local-data-root ".basaltos-local" \
  --payload "$TMP/remote_payload.json" \
  --policy cloud_preferred \
  --apply >/tmp/sync_payload_tooling_import.out

grep -q "\[sync-import\]   updated:" /tmp/sync_payload_tooling_import.out
test -f "$LOCAL/user/presets/preset_cloud_added.json"

echo "PASS: sync payload tooling smoke"
