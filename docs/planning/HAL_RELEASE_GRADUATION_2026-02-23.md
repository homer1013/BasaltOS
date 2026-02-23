# HAL Release Graduation (2026-02-23)

## Graduation criteria snapshot
- HAL adapter coverage complete across tracked ports:
  - Source: `docs/planning/HAL_ADAPTER_MATRIX.md`
- Unsupported stub inventory stays at zero:
  - Source: `docs/planning/HAL_UNSUPPORTED_STUB_INVENTORY.md`
- Per-port maturity tracking now published (`real` vs `contract-only`):
  - `docs/planning/HAL_PORT_MATURITY_REPORT.json`
  - `docs/planning/HAL_PORT_MATURITY_REPORT.md`

## Gate results
- `tools/tests/main_hardening_smoke_bundle.sh`: PASS (`50/50`)
- `tools/tests/non_esp_hero_bringup_smoke.sh`: PASS
- `tools/tests/uno_r4_tft_parallel_bench_smoke.sh`: PASS
- `tools/tests/non_esp_runtime_bench_matrix_drift_smoke.sh`: PASS

## Bench evidence
- Uno R4 parallel TFT bench smoke log:
  - `/tmp/uno_r4_tft_parallel_bench_2026-02-23.log`
- Non-ESP hero bring-up smoke log:
  - `/tmp/non_esp_hero_bringup_2026-02-23.log`

## Outcome
- HAL release tranche is graduation-ready for the current plan scope:
  - Uno R4 parallel TFT runtime path is first-class (`tft_parallel_uno`).
  - Uno runtime includes touch calibration persistence (`cal save/load/clear`).
  - Non-ESP hero bring-up matrix has automated validation coverage.
  - Non-ESP runtime bench matrix publishing is in place (configure always, serial probe where available).
  - HAL maturity report is now tracked with drift protection.
