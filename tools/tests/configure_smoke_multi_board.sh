#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_BASE="tmp/test_configure_smoke_multi_board"
rm -rf "$OUT_BASE"
mkdir -p "$OUT_BASE"

count=0

while IFS='|' read -r platform board_dir; do
  outdir="$OUT_BASE/${platform}/${board_dir}"
  mkdir -p "$outdir"

  echo "[smoke] ${platform}/${board_dir}"
  python3 tools/configure.py \
    --platform "$platform" \
    --board "$board_dir" \
    --outdir "$outdir" \
    >"$outdir/run.log" 2>&1

  test -f "$outdir/basalt_config.h"
  test -f "$outdir/basalt.features.json"
  test -f "$outdir/sdkconfig.defaults"

  count=$((count + 1))
done < <(
  find boards -name board.json -print \
    | sed -E 's#^boards/([^/]+)/([^/]+)/board\.json$#\1|\2#' \
    | sort
)

echo "PASS: configure smoke completed for ${count} boards"
