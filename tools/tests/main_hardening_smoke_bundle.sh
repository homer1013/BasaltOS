#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/51 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/51 configure UX guidance"
bash tools/tests/configure_ux_guidance_smoke.sh

echo "[hardening] 3/51 e2e Playwright harness contract"
bash tools/tests/e2e_playwright_harness_smoke.sh

echo "[hardening] 4/51 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 5/51 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 6/51 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 7/51 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 8/51 board catalog drift"
bash tools/tests/board_catalog_drift_smoke.sh

echo "[hardening] 9/51 board taxonomy fields"
bash tools/tests/board_taxonomy_fields_smoke.sh

echo "[hardening] 10/51 board taxonomy schema"
bash tools/tests/board_taxonomy_schema_smoke.sh

echo "[hardening] 11/51 board taxonomy API contract"
bash tools/tests/board_taxonomy_api_contract_smoke.sh

echo "[hardening] 12/51 driver capability matrix drift"
bash tools/tests/driver_capability_matrix_drift_smoke.sh

echo "[hardening] 13/51 tft posture drift"
bash tools/tests/tft_posture_report_drift_smoke.sh

echo "[hardening] 14/51 twai posture drift"
bash tools/tests/twai_posture_report_drift_smoke.sh

echo "[hardening] 15/51 eeprom posture drift"
bash tools/tests/eeprom_posture_report_drift_smoke.sh

echo "[hardening] 16/51 mcp2544fd posture drift"
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh

echo "[hardening] 17/51 psram posture drift"
bash tools/tests/psram_posture_report_drift_smoke.sh

echo "[hardening] 18/51 tp4056 posture drift"
bash tools/tests/tp4056_posture_report_drift_smoke.sh

echo "[hardening] 19/51 hal adapter matrix drift"
bash tools/tests/hal_adapter_matrix_drift_smoke.sh

echo "[hardening] 20/51 esp32 HAL peripheral contract"
bash tools/tests/esp32_hal_peripheral_contract_smoke.sh

echo "[hardening] 21/51 HAL UART init rollback"
bash tools/tests/hal_uart_init_rollback_smoke.sh

echo "[hardening] 22/51 HAL GPIO IRQ service contract"
bash tools/tests/hal_gpio_irq_service_contract_smoke.sh

echo "[hardening] 23/51 HAL UART driver ownership"
bash tools/tests/hal_uart_driver_ownership_smoke.sh

echo "[hardening] 24/51 HAL I2C timeout semantics"
bash tools/tests/hal_i2c_timeout_semantics_smoke.sh

echo "[hardening] 25/51 HAL I2C driver ownership"
bash tools/tests/hal_i2c_driver_ownership_smoke.sh

echo "[hardening] 26/51 HAL SPI reconfigure safety"
bash tools/tests/hal_spi_reconfigure_safety_smoke.sh

echo "[hardening] 27/51 HAL SPI transfer length bound"
bash tools/tests/hal_spi_transfer_length_bound_smoke.sh

echo "[hardening] 28/51 HAL CMake target contract"
bash tools/tests/hal_cmake_target_contract_smoke.sh

echo "[hardening] 29/51 bus manager usage"
bash tools/tests/bus_manager_usage_smoke.sh

echo "[hardening] 30/51 esp32 HAL error semantics"
bash tools/tests/esp32_hal_error_semantics_smoke.sh

echo "[hardening] 31/51 HAL impl sanity"
bash tools/tests/hal_impl_sanity_smoke.sh

echo "[hardening] 32/51 HAL primitive file integrity"
bash tools/tests/hal_port_primitive_file_integrity_smoke.sh

echo "[hardening] 33/51 HAL no header-style C sources"
bash tools/tests/hal_no_headerstyle_c_sources_smoke.sh

echo "[hardening] 34/51 HAL unsupported stub behavior"
bash tools/tests/hal_unsupported_stub_behavior_smoke.sh

echo "[hardening] 35/51 HAL unsupported stub inventory drift"
bash tools/tests/hal_unsupported_stub_inventory_drift_smoke.sh

echo "[hardening] 36/51 HAL opaque handle assertions"
bash tools/tests/hal_opaque_handle_assert_smoke.sh

echo "[hardening] 37/51 HAL contract policy"
bash tools/tests/hal_contract_policy_smoke.sh

echo "[hardening] 38/51 HAL platform adapter completeness"
bash tools/tests/hal_platform_adapter_completeness_smoke.sh

echo "[hardening] 39/51 driver-to-HAL dependency map drift"
bash tools/tests/driver_hal_dependency_map_drift_smoke.sh

echo "[hardening] 40/51 HAL runtime contract"
bash tools/tests/hal_runtime_contract_smoke.sh

echo "[hardening] 41/51 HAL port maturity drift"
bash tools/tests/hal_port_maturity_report_drift_smoke.sh

echo "[hardening] 42/51 non-ESP runtime bench matrix drift"
bash tools/tests/non_esp_runtime_bench_matrix_drift_smoke.sh

echo "[hardening] 43/51 module picker accessibility"
bash tools/tests/module_picker_accessibility_smoke.sh

echo "[hardening] 44/51 board picker accessibility"
bash tools/tests/board_picker_accessibility_smoke.sh

echo "[hardening] 45/51 configure pin validation smoke"
bash tools/tests/configure_pin_validation_smoke.sh

echo "[hardening] 46/51 configure driver defaults smoke"
bash tools/tests/configure_driver_defaults_smoke.sh

echo "[hardening] 47/51 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "[hardening] 48/51 configure tft_parallel_uno smoke"
bash tools/tests/configure_tft_parallel_uno_smoke.sh

echo "[hardening] 49/51 lua examples parity smoke"
bash tools/tests/lua_examples_parity_smoke.sh

echo "[hardening] 50/51 HAL I/O length bounds"
bash tools/tests/hal_io_length_bounds_smoke.sh

echo "[hardening] 51/51 optional PIC16 live runtime lane"
bash tools/tests/pic16_live_runtime_smoke.sh


echo "PASS: main hardening smoke bundle"
