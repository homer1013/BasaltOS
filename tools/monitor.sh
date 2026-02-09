#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyUSB0}"
DEFAULTS="sdkconfig.defaults;config/generated/sdkconfig.defaults"
SDKCONFIG_DEFAULTS="${SDKCONFIG_DEFAULTS:-$DEFAULTS}" idf.py -p "$PORT" monitor
