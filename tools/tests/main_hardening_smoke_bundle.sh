#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/19 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/19 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 3/19 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 4/19 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 5/19 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 6/19 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 7/19 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 8/19 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 9/19 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 10/19 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 11/19 twai posture drift"
bash tools/tests/twai_posture_report_drift_smoke.sh

echo "[hardening] 12/19 eeprom posture drift"
bash tools/tests/eeprom_posture_report_drift_smoke.sh

echo "[hardening] 13/19 mcp2544fd posture drift"
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh

echo "[hardening] 14/19 psram posture drift"
bash tools/tests/psram_posture_report_drift_smoke.sh

echo "[hardening] 15/19 tp4056 posture drift"
bash tools/tests/tp4056_posture_report_drift_smoke.sh

echo "[hardening] 16/19 hal adapter matrix drift"
bash tools/tests/hal_adapter_matrix_drift_smoke.sh

echo "[hardening] 17/19 HAL contract policy"
bash tools/tests/hal_contract_policy_smoke.sh

echo "[hardening] 18/19 HAL platform adapter completeness"
bash tools/tests/hal_platform_adapter_completeness_smoke.sh

echo "[hardening] 19/19 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
