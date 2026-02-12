#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_metadata_completeness_report/report.md"
rm -rf "$(dirname "$OUT")"

python3 tools/metadata_completeness_report.py --out "$OUT" >/tmp/metadata_completeness_report.out

test -f "$OUT"
grep -q "Board Metadata Completeness Report" "$OUT"
grep -q "Overall completeness" "$OUT"
grep -q "\[metadata\] boards:" /tmp/metadata_completeness_report.out

echo "PASS: metadata completeness report smoke"
