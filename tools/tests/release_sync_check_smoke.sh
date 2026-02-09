#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 tools/release_sync_check.py --self-only --version v0.1.0 >/tmp/release_sync_check.out

grep -q "\[OK\] main.tag: tag exists: v0.1.0" /tmp/release_sync_check.out
grep -q "\[OK\] main.changelog: CHANGELOG has heading for v0.1.0" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS release matches v0.1.0" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS_Platform release matches v0.1.0" /tmp/release_sync_check.out
grep -q "\[OK\] status.doc: BasaltOS_PlatformIO release matches v0.1.0" /tmp/release_sync_check.out

echo "PASS: release sync check smoke"
