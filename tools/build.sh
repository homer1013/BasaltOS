#!/usr/bin/env bash
set -euo pipefail

DEFAULTS="sdkconfig.defaults;config/generated/sdkconfig.defaults"
SDKCONFIG_DEFAULTS="${SDKCONFIG_DEFAULTS:-$DEFAULTS}" idf.py build
