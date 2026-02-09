# PlatformIO Phase-1 Kickoff

This document defines the initial, low-risk PlatformIO integration contract for BasaltOS.

## Objective
Validate that BasaltOS `configure.py` outputs can drive a repeatable PlatformIO ESP-IDF build path.

## Contract Inputs (from main repo)
- `config/generated/basalt.features.json`
- `config/generated/sdkconfig.defaults`
- `config/generated/basalt_config.h`

## Phase-1 Output
Generate a starter PlatformIO project config:
- `platform = espressif32`
- `framework = espidf`
- include path points to `config/generated`
- `board_build.sdkconfig_defaults` points to BasaltOS-generated defaults

Implemented bootstrap tool:
- `tools/platformio/bootstrap_from_features.py`

## Usage
```bash
python3 tools/configure.py --platform esp32 --board esp32-c3-supermini --enable-drivers uart --outdir config/generated
python3 tools/platformio/bootstrap_from_features.py
```

Generated artifacts (default):
- `tmp/platformio_phase1/platformio.ini`
- `tmp/platformio_phase1/README.md`

## Phase-1 Scope (intentionally narrow)
In scope:
- ESP32 only
- Config contract validation
- Board-id heuristic + explicit override support (`--pio-board`)

Out of scope:
- Publishing a custom PlatformIO platform package
- Full multi-platform support in this phase
- VS Code extension features

## Next Step After Kickoff
- Add one reference-board `pio run` CI check once PlatformIO toolchain installation strategy is agreed.
