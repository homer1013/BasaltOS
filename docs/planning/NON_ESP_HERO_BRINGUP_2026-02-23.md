# Non-ESP Hero Bring-up Matrix (2026-02-23)

## Scope
- Validate one hero board each for:
  - `rp2040`
  - `stm32`
  - `pic16`
- Gate requested baseline stack:
  - `gpio`, `uart`, `i2c`, `spi`, `timer`, `pwm`, `shell_full`

## Hero boards
- `rp2040`: `raspberry_pi_pico`
- `stm32`: `nucleo_f446re`
- `pic16`: `curiosity_nano_pic16f13145`

## Automation
- Smoke script:
  - `tools/tests/non_esp_hero_bringup_smoke.sh`
- Evidence log:
  - `/tmp/non_esp_hero_bringup_2026-02-23.log`

## Result
- All three hero boards passed configure + generated-artifact contract checks.
- Generated config reflects the requested stack enabled for each hero board.

## Metadata tightening completed
- `boards/stm32/nucleo_f446re/board.json` now explicitly includes `timer` capability (it was already used in defaults but missing from capability metadata).
