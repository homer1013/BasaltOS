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

## Current blocker to native Basalt `tft` on Uno R4 profile
- With `--allow-unsupported`, `configure.py` currently fails on missing board pin contract keys for `tft`:
  - missing `tft_mosi`, `tft_sclk`, `tft_cs`, `tft_dc`
- This is expected: current Basalt `tft` module is SPI-panel oriented, while the bench shield is 8-bit parallel.

## Next enablement steps
1. Decide runtime target:
   - SPI TFT (supported by current module model), or
   - Parallel UNO shield path (new driver contract needed).
2. If keeping current SPI TFT module:
   - add experimental Uno R4 pin contract for SPI TFT in `boards/renesas_ra/arduino_uno_r4_wifi/board.json`.
3. If supporting this exact shield:
   - define a new `tft_parallel_uno` module contract and runtime backend.
   - add corresponding board pin contract (`lcd_d0..d7`, `lcd_rd`, `lcd_wr`, `lcd_rs`, `lcd_cs`, `lcd_rst`).
4. Connect whichever path is chosen into RA4 runtime shell output surface.
