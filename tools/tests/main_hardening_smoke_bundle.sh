#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/11 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/11 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 3/11 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 4/11 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 5/11 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 6/11 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 7/11 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 8/11 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 9/11 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 10/11 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 11/11 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
