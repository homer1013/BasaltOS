# Changelog

All notable changes to this project are documented in this file.

## Unreleased

### Added
- Shared wizard schema contract endpoints for local configurator: `GET /api/wizard/steps` and `GET /api/wizard/board-filters`.
- New CYD smoke tests:
  - `tools/tests/cyd_config_parity_smoke.sh` (CLI vs API output parity gate)
  - `tools/tests/cyd_esp32_build_smoke.sh` (generate + ESP32 build path)
- HAL completion tranche:
  - all tracked HAL adapter ports now provide concrete primitives (`adc/gpio/i2c/i2s/pwm/rmt/spi/timer/uart`) with no unsupported stub inventory remaining.
  - live Uno R4 WiFi TFT bench validation pass captured with serial and camera evidence (`/dev/video2` bench path).

### Changed
- Local web configurator wizard step container is schema-driven (step labels/count from shared contract), removing hardcoded 4-step control assumptions.
- Board taxonomy filter labels and "all" option text are now sourced from shared board-filter schema for CLI/web parity.
- Configurator API generation now aligns with CLI serializers for applet JSON shape, macro emission, slug ordering, and config payload structure.
- HAL planning and release contracts updated to reflect full adapter completion:
  - `docs/planning/HAL_ADAPTER_MATRIX.*`
  - `docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.*`
  - `docs/planning/HAL_UNSUPPORTED_STUB_INVENTORY.*`
  - `docs/planning/DRIVER_HAL_DEPENDENCY_MAP.*`
  - `docs/planning/HAL_CONTRACT_POLICY.json`

### Fixed
- `tools/configure.py` no longer crashes when `--outdir` is outside repository root.
- ESP32 build no longer fails when partition table omits `storage`; SPIFFS image generation is now conditionally skipped.

## v0.1.2-rc4-configurator-m5-bench-20260224 - 2026-02-24

### Added
- M5StickC Plus2 TFT bench smoke gate: `tools/tests/m5stickc_plus2_tft_bench_smoke.sh` (configure/build/flash + serial assertions).
- ESP32 shell TFT diagnostics command surface:
  - `tft status`
  - `tft clear`
  - `tft fill <black|white|red|green|blue>`
  - `tft text <x> <y> <text>`

### Changed
- ESP32 board bootstrap now asserts board `pwr_hold` pin at startup when defined, improving display/power bring-up on M5StickC Plus2 class boards.
- HAL bench evidence doc extended with M5StickC Plus2 runtime/flash validation details and command-level proof.
- Local configurator UX polish pass:
  - sticky step header/action bar/sidebar behavior,
  - board-required guardrails in steps 2-4 with quick actions,
  - dynamic step labels + flow context/progress cues,
  - Home "Resume Last Session" draft persistence path,
  - quick-pick lane for M5Stack Plus2.

### Fixed
- `tft status` parser behavior in ESP32 shell now handles monitor/serial trailing whitespace cleanly.
- Configurator e2e harness robustness for board selection and direct-step recovery actions.

## v0.1.1 - 2026-02-13

### Added
- ESP32 HAL adapter coverage for `adc`, `i2s`, `rmt`, and new `pwm` implementation, with shared IDF target routing through `basalt_hal/ports/esp32`.
- New shell diagnostics:
  - `i2c status|scan|read`
  - `pwm status|start|duty|freq|stop`
- ESP32-C6 HAL bench smoke expansion for `i2c`, `pwm`, `uart`, `rmt`, and `mic` runtime checks.
- New configurator validation smoke coverage for mode-aware pin rules.

### Changed
- UART module config schema now supports explicit mode selection (`tx_rx`, `tx`, `rx`) for clearer generation and validation behavior.
- Configurator pin validation now enforces mode-aware UART pin requirements and guards against invalid shared TX/RX pin assignments in `tx_rx` mode.
- Driver status inventory in shell now reports expanded foundation drivers including `pwm`, `i2s`, `rmt`, `mic`, and UART diagnostics.

### Fixed
- PWM shell startup flow now avoids unnecessary frequency reprogramming on already-matched runtime frequency, preventing false start failures on ESP32-C6 bench runs.

## v0.1.0 - 2026-02-09

### Added
- Jira workspace automation for daily export/sync/reporting (`jira/scripts/export_daily.sh`, `sync_updates.sh`, blockers/sprint/release/daily reports).
- Public funding metadata for GitHub Sponsors and Buy Me a Coffee (`.github/FUNDING.yml`).
- CLI reliability acceptance matrix for Sprint 2 verification (`docs/CLI_RELIABILITY_ACCEPTANCE.md`).
- Metadata validator regression test coverage in CI (`tools/tests/validate_metadata_report_regression.sh`).

### Changed
- Configurator CI/e2e flow hardened for local-mode scope and market-page visibility behavior.
- Metadata validation tightened with required-field checks, duplicate detection, and remediation reporting (`tools/validate_metadata.py`).
- Release/readiness docs and status reporting aligned to v0.1.0 delivery.

### Fixed
- CI workflow breakages in configurator pipeline and runner compatibility.
- Local configurator navigation guardrails to prevent invalid market/profile paths.
- JIRA sync workflow robustness for create-issue/update operations.

## v0.0.7 - 2026-02-07

### Added
- Board catalog growth across ESP32/AVR/ATmega/SAMD21/RP2040/STM32/Renesas/Linux-SBC profiles.
- Driver catalog expansion including `wifi`, `bluetooth`, `twai`, `mcp2544fd`, `uln2003`, `l298n`, `ads1115`, `mcp23017`, `hp4067`, `tp4056`, and SSD1306/RTC/IMU/DHT22/MIC gates.
- App Market content update including `flappy_bird` packaging and deploy paths.
- E2E configurator test scaffolding under `tools/e2e/`.

### Changed
- CLI wizard alignment with web configurator:
  - board-first taxonomy flow (`Manufacturer -> Architecture -> Family -> Processor/Silicon -> Board`)
  - board-default driver preselection behavior
  - shell alias normalization (`shell_lite` -> `shell_min`, `shell_spiritus` -> `shell_tiny`)
- ESP32-C3 SuperMini metadata now includes TWAI capability and CAN pin mapping (`can_tx`, `can_rx`, optional `can_stby`, `can_en`) so CAN appears in wizard/UI.
- Configurator UX and docs updated for App Market-first app selection and driver terminology consistency.

### Fixed
- AVR SSD1306 custom splash C/C++ linkage build break in generated projects.
- Multiple configurator/runtime edge cases around board capability gating and serial/web flashing flow.
