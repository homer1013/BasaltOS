#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_JSON="${BASALT_PIC16_LIVE_JSON:-docs/planning/NON_ESP_RUNTIME_BENCH_LIVE_2026-02-23.json}"
OUT_MD="${BASALT_PIC16_LIVE_MD:-docs/planning/NON_ESP_RUNTIME_BENCH_LIVE_2026-02-23.md}"

detect_pic16_port() {
  if [[ -n "${BASALT_PIC16_PORT:-}" && -e "${BASALT_PIC16_PORT}" ]]; then
    echo "${BASALT_PIC16_PORT}"
    return 0
  fi
  for p in /dev/ttyACM*; do
    [[ -e "$p" ]] || continue
    if udevadm info -q property -n "$p" 2>/dev/null | rg -q 'ID_VENDOR_ID=03eb'; then
      echo "$p"
      return 0
    fi
  done
  return 1
}

PIC16_PORT="$(detect_pic16_port || true)"
if [[ -z "$PIC16_PORT" ]]; then
  echo "SKIP: no PIC16 Curiosity serial port detected"
  exit 0
fi

DISK_DEV="$(lsblk -pnro NAME,LABEL,TYPE | awk '$3=="disk" && $2=="CURIOSITY" {print $1; exit}')"
if [[ -z "$DISK_DEV" ]]; then
  echo "SKIP: no CURIOSITY disk label detected"
  exit 0
fi

if ! findmnt -rn -S "$DISK_DEV" >/dev/null 2>&1; then
  udisksctl mount -b "$DISK_DEV" >/dev/null
fi

STATUS_FILE="/run/media/$USER/CURIOSITY/STATUS.TXT"
if [[ ! -f "$STATUS_FILE" ]]; then
  echo "SKIP: STATUS.TXT not found at $STATUS_FILE"
  exit 0
fi

export BASALT_PIC16_PORT="$PIC16_PORT"
python3 tools/generate_non_esp_runtime_bench_matrix.py \
  --json-out "$OUT_JSON" \
  --md-out "$OUT_MD" >/tmp/pic16_live_runtime_matrix.log

python3 - "$OUT_JSON" <<'PY'
import json
import sys
from pathlib import Path

obj = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
row = next((r for r in obj.get("rows", []) if r.get("platform") == "pic16"), None)
if not row:
    raise SystemExit("FAIL: pic16 row missing from live matrix")
status = str(row.get("runtime_status") or "")
note = str(row.get("runtime_note") or "")
print(f"pic16 runtime_status={status} note={note}")
if status != "pass":
    raise SystemExit("FAIL: PIC16 live runtime did not pass")
PY

echo "PASS: PIC16 live runtime smoke checks passed ($PIC16_PORT)"
echo "PASS: wrote $OUT_JSON"
echo "PASS: wrote $OUT_MD"
