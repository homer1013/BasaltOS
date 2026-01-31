#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DIST_DIR="${ROOT_DIR}/dist"

FAST=0
if [[ "${1:-}" == "--fast" ]]; then
  FAST=1
fi

echo "[release] Root:      $ROOT_DIR"
echo "[release] Build dir: $BUILD_DIR"
echo "[release] Dist dir:  $DIST_DIR"

mkdir -p "$DIST_DIR"

# Source env if you use it
if [[ -f "$ROOT_DIR/tools/env.sh" ]]; then
  # shellcheck disable=SC1090
  source "$ROOT_DIR/tools/env.sh"
fi

copy_if_exists() {
  local src="$1"
  local dst="$2"
  if [[ -f "$src" ]]; then
    cp -f "$src" "$dst"
    echo "[release] Copied: $(basename "$src") -> $(basename "$dst")"
  else
    echo "[release] Skip (missing): $src"
  fi
}

copy_or_die() {
  local src="$1"
  local dst="$2"
  if [[ ! -f "$src" ]]; then
    echo "[release] ERROR: Missing expected file: $src" >&2
    exit 1
  fi
  cp -f "$src" "$dst"
  echo "[release] Copied: $(basename "$src") -> $(basename "$dst")"
}

echo "[release] Cleaning..."
  if [[ "$FAST" -eq 1 ]]; then
    echo "[release] Skipping fullclean (--fast)"
  else
    echo "[release] Cleaning..."
    idf.py -B "$BUILD_DIR" fullclean
  fi

echo "[release] Building..."
idf.py -B "$BUILD_DIR" build

# Required
copy_or_die "$BUILD_DIR/bootloader/bootloader.bin" "$DIST_DIR/bootloader.bin"
copy_or_die "$BUILD_DIR/partition_table/partition-table.bin" "$DIST_DIR/partition-table.bin"

# App (prefer known name)
if [[ -f "$BUILD_DIR/basaltos.bin" ]]; then
  copy_or_die "$BUILD_DIR/basaltos.bin" "$DIST_DIR/basaltos.bin"
else
  # fallback: pick largest bin in build root excluding partition-table
  APP_BIN="$(ls -1 "$BUILD_DIR"/*.bin 2>/dev/null | grep -v 'partition-table\.bin' | xargs -r ls -S 2>/dev/null | head -n 1 || true)"
  if [[ -z "${APP_BIN:-}" ]]; then
    echo "[release] ERROR: Could not find app .bin in $BUILD_DIR" >&2
    exit 1
  fi
  copy_or_die "$APP_BIN" "$DIST_DIR/app.bin"
fi

# Optional but recommended for "one-command flash"
copy_if_exists "$BUILD_DIR/ota_data_initial.bin" "$DIST_DIR/ota_data_initial.bin"
copy_if_exists "$BUILD_DIR/storage.bin" "$DIST_DIR/storage.bin"
copy_if_exists "$BUILD_DIR/flasher_args.json" "$DIST_DIR/flasher_args.json"

# Handy reproducibility artifacts
copy_if_exists "$ROOT_DIR/sdkconfig" "$DIST_DIR/sdkconfig"
copy_if_exists "$ROOT_DIR/sdkconfig.defaults" "$DIST_DIR/sdkconfig.defaults"

# Build info
{
  echo "BasaltOS release build"
  echo "Date (UTC): $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  echo "IDF: $(idf.py --version 2>/dev/null || true)"
  echo "IDF_PATH: ${IDF_PATH:-"(unknown)"}"
  echo "Git: $(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo "(no git)")"
  echo "App: $(ls -1 "$DIST_DIR"/*.bin 2>/dev/null | xargs -n1 basename | tr '\n' ' ')"
} > "$DIST_DIR/build_info.txt"

echo "[release] Done. dist contents:"
ls -lh "$DIST_DIR"
