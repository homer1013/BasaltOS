# BasaltOS Flexible Priority Queue (No Fixed Dates)

Goal: maximize user adoption and trust before adding more drivers.

## Priority 1: Fast First Success
- Deliver a single canonical "10-minute" quickstart that gets a user from clone -> configure -> flash -> verified shell output.
- Keep one known-good hardware target path (ESP32-C3 SuperMini) with copy/paste commands.
- Include an expected output section and a short troubleshooting block.

## Priority 2: v0.1.1 Stabilization
- Tighten shell UX consistency (help text, usage, errors, command acknowledgements).
- Ensure validated paths produce deterministic output and no confusing partial responses.
- Publish a concise release note focused on "what works now".

## Priority 3: Copy-Paste Examples
- Add practical examples users can run immediately:
  - OLED status example
  - DC motor control example
  - Sensor readout example
- Keep each example to minimal setup and obvious success criteria.

## Priority 4: Configurator UX Hardening
- Improve configure-time messaging and actionable validation errors.
- Ensure board defaults are clear and mismatches are explained without guesswork.
- Add smoke checks for known-good profiles to reduce setup friction.

## Priority 5: Public Progress Cadence
- Add a lightweight weekly changelog/progress routine.
- Keep a release/tag/changelog alignment checklist to avoid drift between GitHub and Jira.
- Include one short status template for social updates (ships + next up + support links).

## Cross-Platform Generation Parity Baseline

- Baseline artifacts:
  - `docs/planning/GENERATION_PARITY_BASELINE.json`
  - `docs/planning/GENERATION_PARITY_BASELINE.md`
- Refresh command:
  - `python3 tools/generate_generation_parity_baseline.py`

## Driver Capability Matrix v1

- Matrix artifacts:
  - `docs/planning/DRIVER_CAPABILITY_MATRIX.csv`
  - `docs/planning/DRIVER_CAPABILITY_MATRIX.json`
  - `docs/planning/DRIVER_CAPABILITY_MATRIX.md`
- Refresh command:
  - `python3 tools/generate_driver_capability_matrix.py`

## Working Style
- Pull next task when current one is done; do not timebox artificially.
- Re-prioritize anytime based on blocker removal or user feedback.
- Prefer "shippable and demonstrable" increments over broad unfinished scope.
