#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GUARD="$ROOT/tools/tests/staged_artifact_guard.sh"

TMP="$(mktemp -d)"
cleanup() {
  rm -rf "$TMP"
}
trap cleanup EXIT

git -C "$TMP" init -q
git -C "$TMP" config user.email "smoke@example.com"
git -C "$TMP" config user.name "Smoke Test"

echo "ok" > "$TMP/README.md"
git -C "$TMP" add README.md
bash "$GUARD" --repo "$TMP" >/dev/null

mkdir -p "$TMP/config/generated"
echo "generated" > "$TMP/config/generated/basalt_config.h"
git -C "$TMP" add config/generated/basalt_config.h
set +e
bash "$GUARD" --repo "$TMP" >/dev/null 2>&1
rc=$?
set -e
if [[ "$rc" -eq 0 ]]; then
  echo "FAIL: staged artifact guard should fail when blocked path is staged"
  exit 1
fi

echo "PASS: staged artifact guard smoke checks passed"
