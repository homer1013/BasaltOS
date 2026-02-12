#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="tmp/test_board_intake_scaffold"
rm -rf "$OUT"
mkdir -p "$OUT"

python3 tools/board_intake_scaffold.py \
  --manufacturer "Adafruit" \
  --family "SAMD21" \
  --platform atsam \
  --board-id "circuit-playground-express" \
  --board-name "Circuit Playground Express" \
  --mcu "ATSAMD21G18" \
  --flash "256KB" \
  --ram "32KB" \
  --output-root "$OUT/pipeline" > "$OUT/run.log"

SCAFF="$OUT/pipeline/adafruit/samd21/circuit_playground_express"
[[ -f "$SCAFF/board.json" ]] || { echo "FAIL: board.json missing"; exit 1; }
[[ -f "$SCAFF/INTAKE.md" ]] || { echo "FAIL: INTAKE.md missing"; exit 1; }
[[ -f "$SCAFF/manifest.json" ]] || { echo "FAIL: manifest.json missing"; exit 1; }

python3 - <<'PY'
import json
from pathlib import Path
p=Path("tmp/test_board_intake_scaffold/pipeline/adafruit/samd21/circuit_playground_express/board.json")
d=json.loads(p.read_text(encoding="utf-8"))
assert d["id"]=="circuit_playground_express"
assert d["platform"]=="atsam"
assert d["manufacturer"]=="Adafruit"
assert d["family"]=="SAMD21"
assert "uart" in d["capabilities"]
assert "pins" in d and isinstance(d["pins"], dict)
print("PASS: board intake scaffold smoke checks passed")
PY
