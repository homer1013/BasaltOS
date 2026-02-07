# Changelog

All notable changes to this project are documented in this file.

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

