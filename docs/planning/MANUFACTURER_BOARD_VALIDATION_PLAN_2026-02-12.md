# Manufacturer Board Validation Plan (2026-02-12)

Scope for `SCRUM-111`.

Primary matrix artifacts (generated):

- `docs/planning/MANUFACTURER_BOARD_MATRIX.csv`
- `docs/planning/MANUFACTURER_BOARD_MATRIX.md`
- `docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.json`
- `docs/planning/MANUFACTURER_PROFILE_CREATION_QUEUE.md`

Generate/update:

```bash
python3 tools/generate_manufacturer_matrix.py
```

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

See generated matrix for current candidate set and readiness status:

- `docs/planning/MANUFACTURER_BOARD_MATRIX.md`

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
