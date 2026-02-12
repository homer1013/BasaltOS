#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_BASE="tmp/test_configure_smoke_multi_board"
rm -rf "$OUT_BASE"
mkdir -p "$OUT_BASE"

count=0
SUMMARY="$OUT_BASE/summary.md"
BOARD_LIST="$OUT_BASE/boards.list"

cat > "$BOARD_LIST" <<'EOF'
EOF

find boards -name board.json -print \
  | sed -E 's#^boards/([^/]+)/([^/]+)/board\.json$#\1|\2#' \
  | sort > "$BOARD_LIST"

declare -A platform_counts=()
required_platforms=(
  atmega
  atsam
  avr
  esp32
  linux-sbc
  pic16
  renesas_ra
  renesas_rx
  rp2040
  stm32
)

while IFS='|' read -r platform board_dir; do
  [[ -z "${platform:-}" ]] && continue
  [[ -z "${board_dir:-}" ]] && continue
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

  platform_counts["$platform"]=$(( ${platform_counts["$platform"]:-0} + 1 ))
  count=$((count + 1))
done < "$BOARD_LIST"

missing=()
for p in "${required_platforms[@]}"; do
  if [[ "${platform_counts[$p]:-0}" -eq 0 ]]; then
    missing+=("$p")
  fi
done

{
  echo "# Configure Smoke Multi-Board Summary"
  echo
  echo "- Total boards validated: $count"
  echo "- Output root: \`$OUT_BASE\`"
  echo
  echo "## Platform Coverage"
  for p in "${required_platforms[@]}"; do
    echo "- $p: ${platform_counts[$p]:-0}"
  done
  if [[ "${#missing[@]}" -gt 0 ]]; then
    echo
    echo "## Missing Required Platforms"
    for p in "${missing[@]}"; do
      echo "- $p"
    done
  fi
} > "$SUMMARY"

if [[ "${#missing[@]}" -gt 0 ]]; then
  echo "FAIL: missing required platform coverage: ${missing[*]}"
  echo "Summary: $SUMMARY"
  exit 1
fi

echo "PASS: configure smoke completed for ${count} boards"
echo "Summary: $SUMMARY"
