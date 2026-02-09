# Next Driver Intake (Top 2)

This document selects the next two high-impact driver targets and defines acceptance criteria before implementation.

## Status Update (2026-02-09)
- `bme280` and `mcp2515` driver intake items from this doc have been implemented at diagnostics level.
- Hardware-focused MCP2515 validation checklist now lives in `docs/MCP2515_HARDWARE_VERIFICATION.md`.
- Next iteration should focus on hardware-verified CAN workflow reliability and bitrate/network matrix testing.

## Selection Criteria
- High hardware usage frequency in hobby/prototyping workflows.
- Strong fit with existing BasaltOS module model and board metadata.
- Testable via deterministic configure + smoke validation.
- Cross-platform leverage where possible.

## Selected Drivers

## 1) `bme280` (temperature / humidity / pressure)
Why this driver:
- Very common environmental sensor for ESP32, RP2040, and STM32 workflows.
- High utility for demos and real telemetry projects.
- Fits existing I2C/SPI capability model.

Proposed module metadata (minimum):
- `id`: `bme280`
- `provides`: `sensor_temp`, `sensor_humidity`, `sensor_pressure`
- `depends`: `i2c` (default) with optional SPI mode later
- `pins`: optional explicit pins when board remap is needed
- `config_schema` fields:
  - `bus` (`i2c` default)
  - `i2c_address` (default `0x76`, optional `0x77`)
  - `sample_rate_hz` (bounded integer)

Acceptance smoke checks:
1. `configure.py` enables `bme280` only when `i2c` is available/capable.
2. Generated `basalt.features.json` includes `bme280` config payload.
3. Invalid address/sample-rate fails with clear error.
4. Runtime probe helper returns sensor-present/sensor-missing status without crash.

## 2) `mcp2515` (SPI CAN controller)
Why this driver:
- Brings CAN workflows to boards without native TWAI/CAN.
- Complements existing `twai` + `mcp2544fd` paths.
- Common low-cost module ecosystem.

Proposed module metadata (minimum):
- `id`: `mcp2515`
- `provides`: `can_controller`
- `depends`: `spi`
- `pins`: `can_cs`, `can_int` (optional `can_rst`)
- `config_schema` fields:
  - `bitrate` (e.g. 125000/250000/500000)
  - `oscillator_hz` (e.g. 8000000 or 16000000)

Acceptance smoke checks:
1. `configure.py` requires `spi` and required pins when module enabled.
2. Board capability gates prevent enable on unsupported boards.
3. Generated config emits pin macros and bitrate/oscillator options.
4. CAN send/receive shell smoke path is available when driver is enabled.

## Board Metadata Requirements (for both drivers)
Before enabling on a board:
- Add capability token(s) needed by the module.
- Add required pins under `pins`.
- Add optional defaults in `defaults.modules` and module config block.
- Add pin descriptions in board docs/comments where present.

## Delivery Plan
1. Add `bme280` module metadata + configure validation + smoke coverage.
2. Add `mcp2515` module metadata + configure validation + smoke coverage.
3. Add one reference board example for each driver.
4. Update docs (`docs/boards.md`, `tools/README.md`) with enable/test commands.
