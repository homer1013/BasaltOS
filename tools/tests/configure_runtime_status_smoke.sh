#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="$(python3 tools/configure.py --list-runtime-status)"
echo "$OUT" | grep -q "gpio"
echo "$OUT" | grep -q "ready"
echo "$OUT" | grep -q "config-only"

echo "PASS: configure runtime status smoke checks"
