#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$ROOT/tmp/release_sync_update_smoke"
mkdir -p "$TMP"

cp "$ROOT/docs/RELEASE_SYNC_STATUS.md" "$TMP/status.md"

python3 "$ROOT/tools/release_sync_update.py" \
  --status-file "$TMP/status.md" \
  --version v0.1.0 \
  --all-status in_progress >/tmp/release_sync_update.out

grep -q "BasaltOS: release=v0.1.0 status=in_progress" /tmp/release_sync_update.out
grep -q "BasaltOS_Platform: release=v0.1.0 status=in_progress" /tmp/release_sync_update.out
grep -q "BasaltOS_PlatformIO: release=v0.1.0 status=in_progress" /tmp/release_sync_update.out

grep -q "| BasaltOS | v0.1.0 | in_progress |" "$TMP/status.md"
grep -q "| BasaltOS_Platform | v0.1.0 | in_progress |" "$TMP/status.md"
grep -q "| BasaltOS_PlatformIO | v0.1.0 | in_progress |" "$TMP/status.md"

echo "PASS: release sync update smoke"
