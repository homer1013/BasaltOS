#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/27 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/27 configure UX guidance"
bash tools/tests/configure_ux_guidance_smoke.sh

echo "[hardening] 3/27 e2e Playwright harness contract"
bash tools/tests/e2e_playwright_harness_smoke.sh

echo "[hardening] 4/27 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 5/27 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 6/27 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 7/27 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 8/27 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 9/27 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 10/27 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 11/27 board taxonomy API contract"
bash tools/tests/board_taxonomy_api_contract_smoke.sh

echo "[hardening] 12/27 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 13/27 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 14/27 twai posture drift"
bash tools/tests/twai_posture_report_drift_smoke.sh

echo "[hardening] 15/27 eeprom posture drift"
bash tools/tests/eeprom_posture_report_drift_smoke.sh

echo "[hardening] 16/27 mcp2544fd posture drift"
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh

echo "[hardening] 17/27 psram posture drift"
bash tools/tests/psram_posture_report_drift_smoke.sh

echo "[hardening] 18/27 tp4056 posture drift"
bash tools/tests/tp4056_posture_report_drift_smoke.sh

echo "[hardening] 19/27 hal adapter matrix drift"
bash tools/tests/hal_adapter_matrix_drift_smoke.sh

echo "[hardening] 20/27 esp32 HAL peripheral contract"
bash tools/tests/esp32_hal_peripheral_contract_smoke.sh

echo "[hardening] 21/27 HAL contract policy"
bash tools/tests/hal_contract_policy_smoke.sh

echo "[hardening] 22/27 HAL platform adapter completeness"
bash tools/tests/hal_platform_adapter_completeness_smoke.sh

echo "[hardening] 23/27 driver-to-HAL dependency map drift"
bash tools/tests/driver_hal_dependency_map_drift_smoke.sh

echo "[hardening] 24/27 HAL runtime contract"
bash tools/tests/hal_runtime_contract_smoke.sh

echo "[hardening] 25/27 module picker accessibility"
bash tools/tests/module_picker_accessibility_smoke.sh

echo "[hardening] 26/27 board picker accessibility"
bash tools/tests/board_picker_accessibility_smoke.sh

echo "[hardening] 27/27 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
