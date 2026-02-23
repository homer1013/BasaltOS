#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/non_esp_runtime_bench_matrix_drift"
mkdir -p "$TMP"

GEN_JSON="$TMP/NON_ESP_RUNTIME_BENCH_MATRIX.generated.json"
GEN_MD="$TMP/NON_ESP_RUNTIME_BENCH_MATRIX.generated.md"

python3 tools/generate_non_esp_runtime_bench_matrix.py \
  --json-out "$GEN_JSON" \
  --md-out "$GEN_MD"

python3 - "$GEN_JSON" <<'PY'
import json
import sys
from pathlib import Path
p = Path(sys.argv[1])
data = json.loads(p.read_text(encoding="utf-8"))
data.pop("generated_utc", None)
p.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
PY

python3 - docs/planning/NON_ESP_RUNTIME_BENCH_MATRIX.json <<'PY'
import json
import sys
from pathlib import Path
p = Path(sys.argv[1])
data = json.loads(p.read_text(encoding="utf-8"))
data.pop("generated_utc", None)
Path("/tmp/non_esp_runtime_bench_ref.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
PY

if ! diff -u /tmp/non_esp_runtime_bench_ref.json "$GEN_JSON" >/tmp/non_esp_runtime_bench_json.diff; then
  echo "FAIL: non-ESP runtime bench JSON drift detected."
  echo "Run: python3 tools/generate_non_esp_runtime_bench_matrix.py"
  cat /tmp/non_esp_runtime_bench_json.diff
  exit 1
fi

if ! diff -u docs/planning/NON_ESP_RUNTIME_BENCH_MATRIX.md "$GEN_MD" >/tmp/non_esp_runtime_bench_md.diff; then
  echo "FAIL: non-ESP runtime bench MD drift detected."
  echo "Run: python3 tools/generate_non_esp_runtime_bench_matrix.py"
  cat /tmp/non_esp_runtime_bench_md.diff
  exit 1
fi

echo "PASS: non-ESP runtime bench matrix drift checks passed"
