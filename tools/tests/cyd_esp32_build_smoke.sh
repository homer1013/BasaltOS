#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_cyd_esp32_build"
rm -rf "$OUT"
mkdir -p "$OUT"

if ! command -v idf.py >/dev/null 2>&1; then
  # Local dev shells may not have ESP-IDF exported yet.
  # shellcheck disable=SC1091
  source tools/env.sh
fi

python3 tools/configure.py \
  --platform esp32 \
  --board cyd_3248s035r \
  --enable-drivers gpio,uart,spi,i2c,fs_spiffs,fs_sd,tft,shell_full \
  --applets tft_hello,system_info,sd_probe \
  --outdir config/generated >"$OUT/configure.log" 2>&1

idf.py -B build fullclean >"$OUT/fullclean.log" 2>&1 || true
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults \
idf.py -B build set-target esp32 >"$OUT/set_target.log" 2>&1
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults \
idf.py -D SDKCONFIG=config/generated/sdkconfig -B build build >"$OUT/build.log" 2>&1 || {
  echo "FAIL: CYD ESP32 build failed"
  echo "--- configure.log ---"
  cat "$OUT/configure.log"
  echo "--- set_target.log ---"
  cat "$OUT/set_target.log"
  echo "--- build.log ---"
  cat "$OUT/build.log"
  exit 1
}

if rg -n "Failed to create SPIFFS image for partition 'storage'" "$OUT/build.log" >/dev/null 2>&1; then
  echo "FAIL: SPIFFS storage partition generation error regressed"
  cat "$OUT/build.log"
  exit 1
fi

echo "PASS: CYD generate + ESP32 build smoke"
