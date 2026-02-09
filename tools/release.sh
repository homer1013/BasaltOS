#!/usr/bin/env bash
set -euo pipefail

# ------------------------------------------------------------
# BasaltOS release builder
# - Builds with ESP-IDF (unless --no-build)
# - Packages flashable artifacts into dist/
# - Adds reproducibility metadata (build_info, hashes, size)
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DIST_ROOT="${ROOT_DIR}/dist"

# Defaults
FAST=0
NO_BUILD=0
TIMESTAMPED=0
TARGET=""
VERSION=""
PORT_DEFAULT="/dev/ttyUSB0"

usage() {
  cat <<EOF
Usage: tools/release.sh [options]

Options:
  --fast                 Skip 'idf.py fullclean' (still builds)
  --no-build             Do not run idf.py; package existing build artifacts
  --target <name>        Run 'idf.py set-target <name>' before build (e.g. esp32, esp32s3)
  --version <tag>        Version label (e.g. v0.1.0) used in build_info and dist naming
  --timestamped          Write outputs to dist/<UTCSTAMP>_<version>/ (no dist wipe)
  --port <device>        Default serial port used by generated dist/flash.sh (default: /dev/ttyUSB0)
  -h, --help             Show this help

Environment:
  IDF_PATH, IDF_TARGET   Standard ESP-IDF environment variables
  PROJECT_BIN            Expected app bin name (without .bin), e.g. basaltos
EOF
}

# Simple arg parsing
while [[ $# -gt 0 ]]; do
  case "$1" in
    --fast) FAST=1; shift ;;
    --no-build) NO_BUILD=1; shift ;;
    --timestamped) TIMESTAMPED=1; shift ;;
    --target) TARGET="${2:-}"; shift 2 ;;
    --version) VERSION="${2:-}"; shift 2 ;;
    --port) PORT_DEFAULT="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *)
      echo "[release] ERROR: Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

# Source env if present
if [[ -f "$ROOT_DIR/tools/env.sh" ]]; then
  # shellcheck disable=SC1090
  source "$ROOT_DIR/tools/env.sh"
fi

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "[release] ERROR: missing '$1' in PATH" >&2
    exit 1
  }
}

# Preflight
require_cmd idf.py
require_cmd python

if ! idf.py --version >/dev/null 2>&1; then
  echo "[release] ERROR: idf.py not functional. Did you export ESP-IDF env? (tools/env.sh?)" >&2
  exit 1
fi

# Dist directory selection
UTCSTAMP="$(date -u +"%Y%m%dT%H%M%SZ")"
SAFE_VER=""
if [[ -n "$VERSION" ]]; then
  # sanitize for folder name (keep alnum, dot, dash, underscore)
  SAFE_VER="$(echo "$VERSION" | tr -cd '[:alnum:]._-' )"
fi

if [[ "$TIMESTAMPED" -eq 1 ]]; then
  if [[ -n "$SAFE_VER" ]]; then
    DIST_DIR="${DIST_ROOT}/${UTCSTAMP}_${SAFE_VER}"
  else
    DIST_DIR="${DIST_ROOT}/${UTCSTAMP}"
  fi
  mkdir -p "$DIST_DIR"
else
  DIST_DIR="${DIST_ROOT}"
  rm -rf "$DIST_DIR"
  mkdir -p "$DIST_DIR"
fi

echo "[release] Root:      $ROOT_DIR"
echo "[release] Build dir: $BUILD_DIR"
echo "[release] Dist dir:  $DIST_DIR"
echo "[release] Options:   FAST=$FAST NO_BUILD=$NO_BUILD TARGET='${TARGET:-}' VERSION='${VERSION:-}' TIMESTAMPED=$TIMESTAMPED"

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

# Optionally set target
if [[ -n "$TARGET" && "$NO_BUILD" -eq 0 ]]; then
  echo "[release] Setting target: $TARGET"
  idf.py -B "$BUILD_DIR" set-target "$TARGET"
fi

# Clean/build
if [[ "$NO_BUILD" -eq 0 ]]; then
  if [[ "$FAST" -eq 1 ]]; then
    echo "[release] Skipping fullclean (--fast)"
  else
    echo "[release] Cleaning build dir..."
    idf.py -B "$BUILD_DIR" fullclean
  fi

  echo "[release] Building..."
  idf.py -B "$BUILD_DIR" build
else
  echo "[release] --no-build set: packaging existing build artifacts only"
  if [[ ! -d "$BUILD_DIR" ]]; then
    echo "[release] ERROR: Build dir does not exist: $BUILD_DIR" >&2
    exit 1
  fi
fi

# Required artifacts
copy_or_die "$BUILD_DIR/bootloader/bootloader.bin" "$DIST_DIR/bootloader.bin"
copy_or_die "$BUILD_DIR/partition_table/partition-table.bin" "$DIST_DIR/partition-table.bin"

# App artifact: prefer explicit PROJECT_BIN, then known basaltos, else safe fallback
APP_NAME="${PROJECT_BIN:-}"
if [[ -z "$APP_NAME" && -f "$BUILD_DIR/basaltos.bin" ]]; then
  APP_NAME="basaltos"
fi

if [[ -n "$APP_NAME" && -f "$BUILD_DIR/${APP_NAME}.bin" ]]; then
  copy_or_die "$BUILD_DIR/${APP_NAME}.bin" "$DIST_DIR/${APP_NAME}.bin"
else
  echo "[release] WARNING: App bin name not deterministic (set PROJECT_BIN to avoid fallback)."
  # Fallback: pick largest .bin excluding known non-app bins
  APP_BIN="$(ls -1 "$BUILD_DIR"/*.bin 2>/dev/null \
    | grep -Ev '(partition-table\.bin|bootloader\.bin|ota_data_initial\.bin|storage\.bin)$' \
    | xargs -r ls -S 2>/dev/null | head -n 1 || true)"

  if [[ -z "${APP_BIN:-}" ]]; then
    echo "[release] ERROR: Could not find app .bin in $BUILD_DIR" >&2
    exit 1
  fi
  copy_or_die "$APP_BIN" "$DIST_DIR/app.bin"
fi

# Optional but recommended for one-command flashing
copy_if_exists "$BUILD_DIR/ota_data_initial.bin" "$DIST_DIR/ota_data_initial.bin"
copy_if_exists "$BUILD_DIR/storage.bin" "$DIST_DIR/storage.bin"
copy_if_exists "$BUILD_DIR/flasher_args.json" "$DIST_DIR/flasher_args.json"

# Reproducibility artifacts
copy_if_exists "$ROOT_DIR/sdkconfig" "$DIST_DIR/sdkconfig"
copy_if_exists "$ROOT_DIR/sdkconfig.defaults" "$DIST_DIR/sdkconfig.defaults"

# IDF size info (best-effort)
if [[ "$NO_BUILD" -eq 0 ]]; then
  idf.py -B "$BUILD_DIR" size > "$DIST_DIR/size.txt" 2>/dev/null || true
  idf.py -B "$BUILD_DIR" size-components > "$DIST_DIR/size_components.txt" 2>/dev/null || true
fi

# Hashes + sizes (best-effort)
( cd "$DIST_DIR" && sha256sum *.bin 2>/dev/null > SHA256SUMS.txt || true )
( cd "$DIST_DIR" && ls -lh *.bin 2>/dev/null > SIZES.txt || true )

# Generate a helper flash script if flasher args exist
if [[ -f "$DIST_DIR/flasher_args.json" ]]; then
  cat > "$DIST_DIR/flash.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail
PORT="\${1:-${PORT_DEFAULT}}"
BAUD="\${BAUD:-460800}"
echo "[flash] Port: \$PORT  Baud: \$BAUD"
python -m esptool --chip auto -p "\$PORT" -b "\$BAUD" write_flash @flasher_args.json
EOF
  chmod +x "$DIST_DIR/flash.sh"
  echo "[release] Generated: flash.sh"
fi

# Build info (rich)
GIT_SHA="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo "(no git)")"
GIT_DIRTY="clean"
if git -C "$ROOT_DIR" diff --quiet >/dev/null 2>&1; then :; else GIT_DIRTY="dirty"; fi

SDK_SHA="$(sha256sum "$ROOT_DIR/sdkconfig" 2>/dev/null | awk '{print $1}' || true)"
IDF_VER="$(idf.py --version 2>/dev/null || true)"
TARGET_INFO="${IDF_TARGET:-}"
if [[ -z "$TARGET_INFO" && -f "$ROOT_DIR/sdkconfig" ]]; then
  # Try to extract from sdkconfig if present
  TARGET_INFO="$(grep -E '^CONFIG_IDF_TARGET=' "$ROOT_DIR/sdkconfig" 2>/dev/null | head -n 1 | cut -d= -f2 | tr -d '"')"
fi

{
  echo "BasaltOS release build"
  echo "Version: ${VERSION:-"(unset)"}"
  echo "Date (UTC): ${UTCSTAMP}"
  echo "IDF: ${IDF_VER:-"(unknown)"}"
  echo "IDF_PATH: ${IDF_PATH:-"(unknown)"}"
  echo "Target: ${TARGET_INFO:-"(unknown)"}"
  echo "Git: ${GIT_SHA} (${GIT_DIRTY})"
  echo "sdkconfig sha256: ${SDK_SHA:-"(none)"}"
  echo "Artifacts:"
  ls -1 "$DIST_DIR" | sed 's/^/  - /'
  echo ""
  echo "Submodules:"
  git -C "$ROOT_DIR" submodule status 2>/dev/null || echo "  (none or not a git checkout)"
} > "$DIST_DIR/build_info.txt"

echo "[release] Done. dist contents:"
ls -lh "$DIST_DIR"
