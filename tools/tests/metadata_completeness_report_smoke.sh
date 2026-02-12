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

# Gate should pass for a realistic threshold.
python3 tools/metadata_completeness_report.py --out "$OUT" --fail-under 95 >/tmp/metadata_completeness_gate_pass.out
grep -q "\[metadata\] gate: PASS" /tmp/metadata_completeness_gate_pass.out

# Gate should fail for impossible threshold.
set +e
python3 tools/metadata_completeness_report.py --out "$OUT" --fail-under 100 >/tmp/metadata_completeness_gate_fail.out 2>&1
rc=$?
set -e
if [[ "$rc" -eq 0 ]]; then
  echo "FAIL: expected metadata completeness gate failure for threshold 100"
  exit 1
fi
grep -q "\[metadata\] gate: FAIL" /tmp/metadata_completeness_gate_fail.out

echo "PASS: metadata completeness report smoke"
