#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/16 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/16 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 3/16 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 4/16 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 5/16 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 6/16 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 7/16 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 8/16 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 9/16 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 10/16 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 11/16 twai posture drift"
bash tools/tests/twai_posture_report_drift_smoke.sh

echo "[hardening] 12/16 eeprom posture drift"
bash tools/tests/eeprom_posture_report_drift_smoke.sh

echo "[hardening] 13/16 mcp2544fd posture drift"
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh

echo "[hardening] 14/16 psram posture drift"
bash tools/tests/psram_posture_report_drift_smoke.sh

echo "[hardening] 15/16 tp4056 posture drift"
bash tools/tests/tp4056_posture_report_drift_smoke.sh

echo "[hardening] 16/16 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
