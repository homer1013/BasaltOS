#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_validate_metadata_report"
rm -rf "$OUT"
mkdir -p "$OUT"

BOARD_FILE="boards/esp32/m5stickc_plus2/board.json"
BACKUP_FILE="$OUT/board_backup.json"
cp "$BOARD_FILE" "$BACKUP_FILE"

restore_board() {
  if [[ -f "$BACKUP_FILE" ]]; then
    cp "$BACKUP_FILE" "$BOARD_FILE"
  fi
}
trap restore_board EXIT

# 1) Baseline pass + report generation.
python3 tools/validate_metadata.py --report "$OUT/pass_report.md" >"$OUT/pass.log" 2>&1
if ! grep -q "Metadata validation passed" "$OUT/pass.log"; then
  echo "FAIL: expected pass output in baseline validation"
  cat "$OUT/pass.log"
  exit 1
fi
if ! grep -q "Validation passed with no metadata errors" "$OUT/pass_report.md"; then
  echo "FAIL: expected no-error marker in baseline report"
  cat "$OUT/pass_report.md"
  exit 1
fi

# 2) Force required-field failure (remove mcu/soc/target_profile) and ensure report captures remediation.
python3 - <<'PY'
import json
from pathlib import Path
p=Path('boards/esp32/m5stickc_plus2/board.json')
d=json.loads(p.read_text(encoding='utf-8'))
d.pop('mcu', None)
d.pop('soc', None)
d.pop('target_profile', None)
p.write_text(json.dumps(d, indent=2), encoding='utf-8')
PY

set +e
python3 tools/validate_metadata.py --report "$OUT/fail_report.md" >"$OUT/fail.log" 2>&1
rc=$?
set -e
if [[ "$rc" -eq 0 ]]; then
  echo "FAIL: expected validation failure after removing required board identity fields"
  cat "$OUT/fail.log"
  exit 1
fi
if ! grep -q "at least one of 'mcu', 'soc', or 'target_profile' is required" "$OUT/fail.log"; then
  echo "FAIL: expected required-field failure message in validator output"
  cat "$OUT/fail.log"
  exit 1
fi
if ! grep -q "Remediation Backlog" "$OUT/fail_report.md"; then
  echo "FAIL: expected remediation backlog section in fail report"
  cat "$OUT/fail_report.md"
  exit 1
fi
if ! grep -q "Add board silicon identity" "$OUT/fail_report.md"; then
  echo "FAIL: expected remediation hint in fail report"
  cat "$OUT/fail_report.md"
  exit 1
fi

echo "PASS: metadata validation report regression checks"
