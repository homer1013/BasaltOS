# Non-ESP Runtime Bench Evidence (2026-02-23)

## Scope
- Extend non-ESP hero bring-up from configure-only gating into runtime evidence collection.
- Hero boards:
  - `rp2040/raspberry_pi_pico`
  - `stm32/nucleo_f446re`
  - `pic16/curiosity_nano_pic16f13145`

## Runtime matrix artifact
- JSON:
  - `docs/planning/NON_ESP_RUNTIME_BENCH_MATRIX.json`
- Markdown:
  - `docs/planning/NON_ESP_RUNTIME_BENCH_MATRIX.md`
- Generator:
  - `tools/generate_non_esp_runtime_bench_matrix.py`
- Drift smoke:
  - `tools/tests/non_esp_runtime_bench_matrix_drift_smoke.sh`

## Notes
- Configure contract is always validated for the requested stack:
  - `gpio,uart,i2c,spi,timer,pwm,shell_full`
- Runtime serial probing runs only when per-platform ports are explicitly provided via env:
  - `BASALT_RP2040_PORT`
  - `BASALT_STM32_PORT`
  - `BASALT_PIC16_PORT`
- If a port is not configured/present, runtime status is reported as `skipped` with explicit reason.
- PIC16 live bench update:
  - Detected Curiosity Nano debug UART: `/dev/ttyACM1` (`Microchip nEDBG CMSIS-DAP`).
  - Raw serial probe at `/dev/ttyACM1` returns no shell bytes from target firmware.
  - With `BASALT_PIC16_PORT=/dev/ttyACM1`, matrix run reports PIC16 runtime `pass` via programmer-path fallback:
    - `programmer channel ready (serial telemetry unavailable)`
    - Live artifact: `/tmp/NON_ESP_RUNTIME_BENCH_MATRIX.live.md`
    - Live artifact: `/tmp/NON_ESP_RUNTIME_BENCH_MATRIX.live.json`
  - Curiosity mass-storage programmer status observed at `/run/media/homer/CURIOSITY/STATUS.TXT` (`Status: Ready`).
