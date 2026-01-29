#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/runtime/python/embed_build"
OUT_DIR="$ROOT_DIR/runtime/python/micropython_embed"

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "missing $BUILD_DIR"
  exit 1
fi

make -C "$BUILD_DIR"
rm -rf "$OUT_DIR"
cp -R "$BUILD_DIR/micropython_embed" "$OUT_DIR"
cp "$BUILD_DIR/mpconfigport.h" "$OUT_DIR/mpconfigport.h"

# Copy Basalt custom module into generated package
cp "$ROOT_DIR/runtime/python/basalt_mod/modbasalt.c" "$OUT_DIR/port/modbasalt.c"
cp "$ROOT_DIR/runtime/python/basalt_mod/board_config.h" "$OUT_DIR/port/board_config.h"

# Ensure component CMakeLists.txt exists
cp "$ROOT_DIR/runtime/python/micropython_embed_component.cmake" "$OUT_DIR/CMakeLists.txt"

echo "MicroPython embed package refreshed at $OUT_DIR"
