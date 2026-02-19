# Tools

## Available tools

- `env.sh` – set up ESP-IDF environment
- `board.sh` – list/select board profiles; applies profile-ready boards (sdkconfig + partitions)
- `board_intake_scaffold.py` – scaffold deterministic board intake workspace (`vendor/family/board`) with starter `board.json`
- `validate_metadata.py` – validate board/module JSON schema and cross-references
- `build.sh` – wrapper for idf.py build
- `flash.sh` – wrapper for idf.py flash
- `monitor.sh` – wrapper for idf.py monitor
- `pic16_curiosity_nano_run.sh` – PIC16F13145 Curiosity Nano helper (XC8 compile + drag-and-drop flash)
- `gen_mpy_embed.sh` – build MicroPython embed sources
- `pack_app.py` – create store-only zip app packages
- `validate_app.py` – validate app folders/zip packages before upload/install
- `new_app.py` – create a new app skeleton (app.toml + main.py)
- `basaltos_config_server.py` – local configurator backend + App Market API
- `platformio/bootstrap_from_features.py` – generate a PlatformIO phase-1 starter config from `config/generated/basalt.features.json`
- `export_local_sync_payload.py` – export local guest data to schema-versioned sync payload (`docs/GUEST_SYNC_CONTRACT.md`)
- `diff_local_sync_payload.py` – compare local vs remote payloads by item id/hash
- `import_local_sync_payload.py` – import remote payloads into local workspace with conflict policy
- `metadata_completeness_report.py` – generate board metadata completeness markdown report
- `generate_board_catalog.py` – generate deterministic board inventory docs (`docs/BOARD_CATALOG.md`, `docs/BOARD_TAXONOMY_INDEX.json`, `docs/BOARD_TAXONOMY_INDEX.csv`)
- `generate_manufacturer_matrix.py` – generate manufacturer starter matrix + profile queue docs (`docs/planning/MANUFACTURER_BOARD_MATRIX.csv`, `docs/planning/MANUFACTURER_BOARD_MATRIX.md`, `docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.json`, `docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.md`)
- `generate_generation_parity_baseline.py` – generate representative cross-platform configure parity docs in two modes (`empty`, `with_defaults`), including module/default-module comparisons and parity gap summary (`docs/planning/GENERATION_PARITY_BASELINE.json`, `docs/planning/GENERATION_PARITY_BASELINE.md`)
- `generate_driver_capability_matrix.py` – generate board-by-driver capability matrix + gap report artifacts (`docs/planning/DRIVER_CAPABILITY_MATRIX.csv`, `docs/planning/DRIVER_CAPABILITY_MATRIX.json`, `docs/planning/DRIVER_CAPABILITY_MATRIX.md`, `docs/planning/DRIVER_CAPABILITY_GAPS.json`, `docs/planning/DRIVER_CAPABILITY_GAPS.md`)
- `generate_tft_posture_report.py` – generate TFT capability posture policy report (`docs/planning/TFT_CAPABILITY_POSTURE.json`, `docs/planning/TFT_CAPABILITY_POSTURE.md`)
- `generate_twai_posture_report.py` – generate TWAI capability posture policy report (`docs/planning/TWAI_CAPABILITY_POSTURE.json`, `docs/planning/TWAI_CAPABILITY_POSTURE.md`)
- `generate_eeprom_posture_report.py` – generate EEPROM capability posture policy report (`docs/planning/EEPROM_CAPABILITY_POSTURE.json`, `docs/planning/EEPROM_CAPABILITY_POSTURE.md`)
- `generate_mcp2544fd_posture_report.py` – generate MCP2544FD capability posture policy report (`docs/planning/MCP2544FD_CAPABILITY_POSTURE.json`, `docs/planning/MCP2544FD_CAPABILITY_POSTURE.md`)
- `generate_psram_posture_report.py` – generate PSRAM capability posture policy report (`docs/planning/PSRAM_CAPABILITY_POSTURE.json`, `docs/planning/PSRAM_CAPABILITY_POSTURE.md`)
- `generate_tp4056_posture_report.py` – generate TP4056 capability posture policy report (`docs/planning/TP4056_CAPABILITY_POSTURE.json`, `docs/planning/TP4056_CAPABILITY_POSTURE.md`)
- `generate_hal_adapter_matrix.py` – generate HAL port adapter coverage matrix artifacts (`docs/planning/HAL_ADAPTER_MATRIX.csv`, `docs/planning/HAL_ADAPTER_MATRIX.json`, `docs/planning/HAL_ADAPTER_MATRIX.md`)
- `generate_hal_platform_adapter_completeness.py` – generate required platform adapter completeness artifacts (`docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json`, `docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.md`)
- `generate_driver_hal_dependency_map.py` – generate module dependency-to-HAL mapping and coverage risk artifacts (`docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json`, `docs/planning/DRIVER_HAL_DEPENDENCY_MAP.md`)
- `generate_hal_runtime_contract_report.py` – generate runtime init/status contract coverage report for required HAL ports (`docs/planning/HAL_RUNTIME_CONTRACT_REPORT.json`, `docs/planning/HAL_RUNTIME_CONTRACT_REPORT.md`)
- `release_sync_check.py` – validate release/tag/changelog alignment across BasaltOS repos
- `release_sync_update.py` – update release sync status rows in one command

## Configurator flow notes

- CLI wizard (`python tools/configure.py --wizard`) is the canonical flow contract.
- CLI quickstart (`python tools/configure.py --quickstart <profile-or-board>`) provides first-success presets.
- Profile commands:
  - `python tools/configure.py --list-profiles`
  - `python tools/configure.py --profile-load <id-or-json-path>`
  - `python tools/configure.py --profile-save <id-or-json-path>`
- Runtime readiness surface:
  - `python tools/configure.py --list-runtime-status`
  - generated runs print runtime readiness per enabled driver
- Local web configurator mirrors the wizard step model and board taxonomy filters.
- Runtime options (applets/market apps) are explicit in step 3.
- Pin mapping is optional/advanced; default board pins are used unless overridden.

ESP-IDF build commands should run from repo root (`BasaltOS_Main`), not `tools/`.

## new_app.py

Create a new app under `apps/`:
```
python tools/new_app.py blink_rgb
```

Create a new app under a custom folder:
```
python tools/new_app.py blink_rgb --dest my_apps
```

Set a display title for app.toml:
```
python tools/new_app.py blink_rgb --title "Blink RGB"
```

Create a Lua app skeleton:
```
python tools/new_app.py blink_lua --runtime lua
```

## validate_app.py

Validate an app folder:
```
python tools/validate_app.py apps/example.app
```

Validate a zip package:
```
python tools/validate_app.py dist/example.zip
```

Optional syntax check (best-effort with CPython):
```
python tools/validate_app.py apps/example.app --check-syntax
```

## pack_app.py

Pack an app into a store-only zip:
```
python tools/pack_app.py apps/example.app dist/example.zip
```

Pack with syntax validation:
```
python tools/pack_app.py apps/example.app dist/example.zip --check-syntax
```

## App Market API (local)

When running `python tools/basaltos_config_server.py`, these endpoints are available:

- `GET /api/market/catalog?platform=<id>` – list curated market apps
- `GET /api/market/download/<app_id>?platform=<id>` – download app package zip
- `POST /api/market/upload` – upload app package zip and upsert local catalog entry
  - multipart fields:
    - `package` (required zip)
    - `id`, `name`, `description`, `version`, `author`, `platforms` (optional metadata)
  - server validates package structure + app entry before accepting upload

## CLI Regression Tests

Metadata validator regression (report mode + required-field enforcement):

```bash
bash tools/tests/validate_metadata_report_regression.sh
```

Invalid-board hard-fail regression guard:

```bash
bash tools/tests/configure_invalid_board_regression.sh
```

Configurator UX guidance smoke (error/suggestion/default guidance):

```bash
bash tools/tests/configure_ux_guidance_smoke.sh
```

Negative-path CLI validation (template/platform/driver/input failures):

```bash
bash tools/tests/configure_negative_paths.sh
```

Multi-board CLI smoke run (all discovered boards):

```bash
bash tools/tests/configure_smoke_multi_board.sh
```

Deterministic generation check (normalizes `generated_utc` JSON field before diff):

```bash
bash tools/tests/configure_deterministic_outputs.sh
```

Configurator API smoke test:

```bash
bash tools/tests/configurator_api_smoke.sh
```

CYD CLI/API parity smoke (web generate output must match CLI output files):

```bash
bash tools/tests/cyd_config_parity_smoke.sh
```

CYD generate + ESP32 build smoke (guards partition/SPIFFS regressions):

```bash
bash tools/tests/cyd_esp32_build_smoke.sh
```

CLI quickstart/profile smoke:

```bash
bash tools/tests/configure_quickstart_profile_smoke.sh
```

Runtime readiness listing smoke:

```bash
bash tools/tests/configure_runtime_status_smoke.sh
```

Local-mode UI/API guard test (market/profile hidden + market API disabled):

```bash
node tools/e2e/local_mode_nav_guard.js
```

Playwright e2e harness setup smoke:

```bash
bash tools/tests/e2e_playwright_harness_smoke.sh
```

Local sync payload export smoke:

```bash
bash tools/tests/export_local_sync_payload_smoke.sh
```

Sync payload tooling smoke (export + diff + import):

```bash
bash tools/tests/sync_payload_tooling_smoke.sh
```

Metadata completeness report smoke:

```bash
bash tools/tests/metadata_completeness_report_smoke.sh
```

Metadata completeness gate (example threshold):

```bash
python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md
```

Metadata taxonomy drift smoke (board filter dimensions):

```bash
bash tools/tests/metadata_taxonomy_drift_smoke.sh
```

Manufacturer candidate validation smoke (existing profiles + planned gaps):

```bash
bash tools/tests/manufacturer_candidate_validation_smoke.sh
```

Board catalog drift smoke (docs inventory stays in sync):

```bash
bash tools/tests/board_catalog_drift_smoke.sh
```

Board taxonomy explicit-fields smoke (manufacturer/architecture/family):

```bash
bash tools/tests/board_taxonomy_fields_smoke.sh
```

Board taxonomy schema smoke (id/value/index consistency):

```bash
bash tools/tests/board_taxonomy_schema_smoke.sh
```

Board taxonomy API contract smoke:

```bash
bash tools/tests/board_taxonomy_api_contract_smoke.sh
```

Board intake scaffold smoke:

```bash
bash tools/tests/board_intake_scaffold_smoke.sh
```

Manufacturer starter matrix drift smoke:

```bash
bash tools/tests/manufacturer_matrix_drift_smoke.sh
```

Cross-platform generation parity baseline drift smoke:

```bash
bash tools/tests/generation_parity_baseline_drift_smoke.sh
```

Cross-platform generation parity defaults gate smoke:

```bash
bash tools/tests/generation_parity_defaults_gate_smoke.sh
```

Driver capability matrix drift smoke:

```bash
bash tools/tests/driver_capability_matrix_drift_smoke.sh
```

TFT posture report drift smoke:

```bash
bash tools/tests/tft_posture_report_drift_smoke.sh
```

TWAI posture report drift smoke:

```bash
bash tools/tests/twai_posture_report_drift_smoke.sh
```

EEPROM posture report drift smoke:

```bash
bash tools/tests/eeprom_posture_report_drift_smoke.sh
```

MCP2544FD posture report drift smoke:

```bash
bash tools/tests/mcp2544fd_posture_report_drift_smoke.sh
```

PSRAM posture report drift smoke:

```bash
bash tools/tests/psram_posture_report_drift_smoke.sh
```

TP4056 posture report drift smoke:

```bash
bash tools/tests/tp4056_posture_report_drift_smoke.sh
```

HAL adapter matrix drift smoke:

```bash
bash tools/tests/hal_adapter_matrix_drift_smoke.sh
```

HAL contract policy smoke:

```bash
bash tools/tests/hal_contract_policy_smoke.sh
```

HAL platform adapter completeness smoke:

```bash
bash tools/tests/hal_platform_adapter_completeness_smoke.sh
```

Driver-to-HAL dependency map drift smoke:

```bash
bash tools/tests/driver_hal_dependency_map_drift_smoke.sh
```

HAL runtime contract smoke:

```bash
bash tools/tests/hal_runtime_contract_smoke.sh
```


## PlatformIO Phase-1 Bootstrap

```bash
python3 tools/platformio/bootstrap_from_features.py
```

Override board id if needed:

```bash
python3 tools/platformio/bootstrap_from_features.py --pio-board esp32-c3-devkitm-1
```

PlatformIO phase-1 contract smoke (bootstrap from generated features):

```bash
bash tools/tests/platformio_phase1_contract.sh
```

BME280 driver configure smoke:

```bash
bash tools/tests/configure_bme280_smoke.sh
```

MCP2515 driver configure smoke:

```bash
bash tools/tests/configure_mcp2515_smoke.sh
```


Release sync checker (self-only, CI-safe):

```bash
python3 tools/release_sync_check.py --self-only --version v0.1.0
```

Cross-repo release sync check (local workspace with sibling repos):

```bash
python3 tools/release_sync_check.py --version v0.1.0
```

Release sync checker smoke test:

```bash
bash tools/tests/release_sync_check_smoke.sh
```


Release sync status updater:

```bash
python3 tools/release_sync_update.py --version v0.1.1 --all-status in_progress
python3 tools/release_sync_update.py --version v0.1.1 --main-status synced --platform-status synced --platformio-status in_progress
```

Release sync updater smoke test:

```bash
bash tools/tests/release_sync_update_smoke.sh
```

## PIC16 Curiosity Nano Helper

End-to-end helper for local PIC16 bench flow (XC8 + DFP + drag-drop flash):

```bash
bash tools/pic16_curiosity_nano_run.sh
```

Compile only (no flash):

```bash
bash tools/pic16_curiosity_nano_run.sh --no-flash
```
