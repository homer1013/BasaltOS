# Manufacturer Board Validation Plan (2026-02-12)

Scope for `SCRUM-111`.

## Constraint

- In-hand bench testing currently available for one manufacturer board:
  - Adafruit Circuit Playground Express (ATSAMD21)

## Two-Lane Execution Model

1. Metadata/config-smoke lane:
   - Run configure smoke for candidate boards that already have `board.json`.
   - Track missing manufacturer candidates as planned profile gaps.
2. Bench lane:
   - Execute hardware bench validation only for in-hand board(s).
   - Keep all non-in-hand boards marked metadata-only until hardware is available.

## Candidate Set

- Adafruit:
  - `atsam/adafruit_circuit_playground_express` (bench lane + metadata lane)
  - `rp2040/adafruit_feather_rp2040` (metadata lane)
- Seeed Studio:
  - `atsam/seeeduino_xiao_samd21` (metadata lane)
- SparkFun:
  - `rp2040/sparkfun_thing_plus_rp2040` (metadata lane; profile added)
- DF Robot:
  - `esp32/dfrobot_firebeetle_esp32` (metadata lane; profile added)
- Elecrow:
  - `esp32/elecrow_crowpanel_esp32` (metadata lane; profile added)

## Command

```bash
bash tools/tests/manufacturer_candidate_validation_smoke.sh
```

Output:

- `tmp/manufacturer_candidate_smoke/summary.md`

## Done Criteria

- Existing candidate profiles pass configure smoke.
- Missing candidate profiles are explicitly listed and tracked.
- Bench note captures in-hand hardware coverage limits.
