# RA4 TFT Enablement Slice (2026-02-23)

## Goal
Start practical enablement work from confirmed Arduino Uno R4 WiFi + 3.5" resistive TFT bench success into Basalt configuration/runtime paths.

## Completed in this slice
- Confirmed Uno R4 hardware bench viability with local patched `MCUFRIEND_kbv` path:
  - LCD ID detected: `0x6814`
  - Touch path validated with live raw readings
  - Bench shell app supports paint, touch diagnostics, calibration, and flappy demo:
    - `tools/bench/uno_r4_tft_shell_paint/uno_r4_tft_shell_paint.ino`
- Added explicit experimental override switch in configurator:
  - `tools/configure.py`: `--allow-unsupported`
  - Behavior:
    - Without flag: stops at board capability gate.
    - With flag: bypasses capability gate and continues to next validation layer.
- Added regression smoke for override behavior:
  - `tools/tests/configure_allow_unsupported_smoke.sh`
- Added native parallel shield contract path for Uno R4:
  - new module contract: `modules/tft_parallel_uno/module.json`
  - Uno R4 board capability + pin contract for parallel TFT:
    - `boards/renesas_ra/arduino_uno_r4_wifi/board.json`
  - configure success smoke:
    - `tools/tests/configure_tft_parallel_uno_smoke.sh`
- Added dedicated Uno-parallel runtime backend sketch:
  - `modules/tft_parallel_uno/runtime/uno_r4_tft_parallel_runtime/uno_r4_tft_parallel_runtime.ino`
  - Includes calibration persistence command surface:
    - `cal show|reset|set x0 x1 y0 y1|save|load|clear`
  - Calibration persistence is stored in EEPROM-backed struct (`BCAL` magic, versioned payload).
- Added end-to-end Uno bench validation gate:
  - `tools/tests/uno_r4_tft_parallel_bench_smoke.sh`
  - Validates compile + upload + serial command contract (`help`, `lcd id`, `fill red`, `fill blue`)
  - Also validates calibration persistence round-trip (`cal save`, `cal load`).
  - Supports optional visual assertion hook when camera + `ffmpeg` are available:
    - `BASALT_CAMERA_DEVICE=/dev/videoN`

## Current state
- Basalt now has a first-class Uno-parallel module contract (`tft_parallel_uno`) so Uno R4 + MCUFRIEND-style shield configuration no longer depends on `--allow-unsupported`.
- The existing `tft` module remains SPI-panel oriented; Uno-parallel flow should use `tft_parallel_uno`.

## Validation evidence
- Configure contract:
  - `/tmp/config_tft_parallel_uno_2026-02-23.log`
- Uno R4 runtime bench smoke:
  - `/tmp/uno_r4_tft_parallel_bench_2026-02-23.log`
  - Observed serial contract includes `lcd id: 0x6814` and `ok: fill red|blue`.

## Next enablement steps
1. Split touch calibration math into shared helper for reuse by future RA boards.
2. Add deterministic camera backend dependency in CI image to activate visual assertions by default.
