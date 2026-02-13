#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$ROOT/tmp/release_sync_update_smoke"
mkdir -p "$TMP"

cp "$ROOT/docs/RELEASE_SYNC_STATUS.md" "$TMP/status.md"

SMOKE_VERSION="v9.9.9-smoke"

python3 "$ROOT/tools/release_sync_update.py" \
  --status-file "$TMP/status.md" \
  --version "$SMOKE_VERSION" \
  --all-status in_progress >/tmp/release_sync_update.out

grep -q "BasaltOS: release=$SMOKE_VERSION status=in_progress" /tmp/release_sync_update.out
grep -q "BasaltOS_Platform: release=$SMOKE_VERSION status=in_progress" /tmp/release_sync_update.out
grep -q "BasaltOS_PlatformIO: release=$SMOKE_VERSION status=in_progress" /tmp/release_sync_update.out

grep -q "| BasaltOS | $SMOKE_VERSION | in_progress |" "$TMP/status.md"
grep -q "| BasaltOS_Platform | $SMOKE_VERSION | in_progress |" "$TMP/status.md"
grep -q "| BasaltOS_PlatformIO | $SMOKE_VERSION | in_progress |" "$TMP/status.md"

echo "PASS: release sync update smoke"
