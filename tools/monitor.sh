#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyUSB0}"

idf.py -p "$PORT" monitor
