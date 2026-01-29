#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: tools/board.sh <board-name>"
  exit 1
fi

BOARD="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE_DIR="$ROOT_DIR/boards/$BOARD"

if [[ ! -d "$PROFILE_DIR" ]]; then
  echo "unknown board: $BOARD"
  exit 1
fi

if [[ ! -f "$PROFILE_DIR/sdkconfig.defaults" ]]; then
  echo "missing $PROFILE_DIR/sdkconfig.defaults"
  exit 1
fi

if [[ ! -f "$PROFILE_DIR/partitions.csv" ]]; then
  echo "missing $PROFILE_DIR/partitions.csv"
  exit 1
fi

cp "$PROFILE_DIR/sdkconfig.defaults" "$ROOT_DIR/sdkconfig.defaults"
cp "$PROFILE_DIR/partitions.csv" "$ROOT_DIR/partitions.csv"

if [[ -f "$PROFILE_DIR/board_config.h" ]]; then
  cp "$PROFILE_DIR/board_config.h" "$ROOT_DIR/runtime/python/basalt_mod/board_config.h"
fi

echo "Board profile set to: $BOARD"
