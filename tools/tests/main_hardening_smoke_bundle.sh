#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/50 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/50 configure UX guidance"
bash tools/tests/configure_ux_guidance_smoke.sh

echo "[hardening] 3/50 e2e Playwright harness contract"
bash tools/tests/e2e_playwright_harness_smoke.sh

echo "[hardening] 4/50 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 5/50 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 6/50 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 7/50 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 8/50 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 9/50 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 10/50 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 11/50 board taxonomy API contract"
bash tools/tests/board_taxonomy_api_contract_smoke.sh

echo "[hardening] 12/50 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 13/50 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 14/50 twai posture drift"
bash tools/tests/twai_posture_report_drift_smoke.sh

echo "[hardening] 15/50 eeprom posture drift"
bash tools/tests/eeprom_posture_report_drift_smoke.sh

echo "[hardening] 16/50 mcp2544fd posture drift"
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh

echo "[hardening] 17/50 psram posture drift"
bash tools/tests/psram_posture_report_drift_smoke.sh

echo "[hardening] 18/50 tp4056 posture drift"
bash tools/tests/tp4056_posture_report_drift_smoke.sh

echo "[hardening] 19/50 hal adapter matrix drift"
bash tools/tests/hal_adapter_matrix_drift_smoke.sh

echo "[hardening] 20/50 esp32 HAL peripheral contract"
bash tools/tests/esp32_hal_peripheral_contract_smoke.sh

echo "[hardening] 21/50 HAL UART init rollback"
bash tools/tests/hal_uart_init_rollback_smoke.sh

echo "[hardening] 22/50 HAL GPIO IRQ service contract"
bash tools/tests/hal_gpio_irq_service_contract_smoke.sh

echo "[hardening] 23/50 HAL UART driver ownership"
bash tools/tests/hal_uart_driver_ownership_smoke.sh

echo "[hardening] 24/50 HAL I2C timeout semantics"
bash tools/tests/hal_i2c_timeout_semantics_smoke.sh

echo "[hardening] 25/50 HAL I2C driver ownership"
bash tools/tests/hal_i2c_driver_ownership_smoke.sh

echo "[hardening] 26/50 HAL SPI reconfigure safety"
bash tools/tests/hal_spi_reconfigure_safety_smoke.sh

echo "[hardening] 27/50 HAL SPI transfer length bound"
bash tools/tests/hal_spi_transfer_length_bound_smoke.sh

echo "[hardening] 28/50 HAL CMake target contract"
bash tools/tests/hal_cmake_target_contract_smoke.sh

echo "[hardening] 29/50 bus manager usage"
bash tools/tests/bus_manager_usage_smoke.sh

echo "[hardening] 30/50 esp32 HAL error semantics"
bash tools/tests/esp32_hal_error_semantics_smoke.sh

echo "[hardening] 31/50 HAL impl sanity"
bash tools/tests/hal_impl_sanity_smoke.sh

echo "[hardening] 32/50 HAL primitive file integrity"
bash tools/tests/hal_port_primitive_file_integrity_smoke.sh

echo "[hardening] 33/50 HAL no header-style C sources"
bash tools/tests/hal_no_headerstyle_c_sources_smoke.sh

echo "[hardening] 34/50 HAL unsupported stub behavior"
bash tools/tests/hal_unsupported_stub_behavior_smoke.sh

echo "[hardening] 35/50 HAL unsupported stub inventory drift"
bash tools/tests/hal_unsupported_stub_inventory_drift_smoke.sh

echo "[hardening] 36/50 HAL opaque handle assertions"
bash tools/tests/hal_opaque_handle_assert_smoke.sh

echo "[hardening] 37/50 HAL contract policy"
bash tools/tests/hal_contract_policy_smoke.sh

echo "[hardening] 38/50 HAL platform adapter completeness"
bash tools/tests/hal_platform_adapter_completeness_smoke.sh

echo "[hardening] 39/50 driver-to-HAL dependency map drift"
bash tools/tests/driver_hal_dependency_map_drift_smoke.sh

echo "[hardening] 40/50 HAL runtime contract"
bash tools/tests/hal_runtime_contract_smoke.sh

echo "[hardening] 41/50 HAL port maturity drift"
bash tools/tests/hal_port_maturity_report_drift_smoke.sh

echo "[hardening] 42/50 non-ESP runtime bench matrix drift"
bash tools/tests/non_esp_runtime_bench_matrix_drift_smoke.sh

echo "[hardening] 43/50 module picker accessibility"
bash tools/tests/module_picker_accessibility_smoke.sh

echo "[hardening] 44/50 board picker accessibility"
bash tools/tests/board_picker_accessibility_smoke.sh

echo "[hardening] 45/50 configure pin validation smoke"
bash tools/tests/configure_pin_validation_smoke.sh

echo "[hardening] 46/50 configure driver defaults smoke"
bash tools/tests/configure_driver_defaults_smoke.sh

echo "[hardening] 47/50 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "[hardening] 48/50 configure tft_parallel_uno smoke"
bash tools/tests/configure_tft_parallel_uno_smoke.sh

echo "[hardening] 49/50 lua examples parity smoke"
bash tools/tests/lua_examples_parity_smoke.sh

echo "[hardening] 50/50 HAL I/O length bounds"
bash tools/tests/hal_io_length_bounds_smoke.sh


echo "PASS: main hardening smoke bundle"
