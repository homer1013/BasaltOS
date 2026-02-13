# Changelog

All notable changes to this project are documented in this file.

## Unreleased

### Added
- Placeholder for next development cycle.

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
