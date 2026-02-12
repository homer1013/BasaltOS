#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/9 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/9 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 3/9 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 4/9 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 5/9 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 6/9 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 7/9 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 8/9 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 9/9 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
