# Board Onboarding Checklist

Use this checklist when adding a new board and/or onboarding a new manufacturer.

## 1) Manufacturer Intake

- Confirm manufacturer display name (example: `Adafruit`, `Seeed Studio`, `SparkFun`, `DF Robot`, `Elecrow`).
- Capture manufacturer homepage URL.
- Capture canonical naming pattern for boards (how IDs should be normalized).
- List 3-5 initial target boards for BasaltOS intake.
- Record expected chip/platform families (ESP32, RP2040, STM32, AVR, etc.).

## 2) Board Candidate Selection

- Pick one board as the first intake candidate.
- Verify board has accessible docs (pinout, memory, MCU/SOC, peripherals).
- Verify at least one practical BasaltOS use case (shell, sensor, display, comms).
- Confirm board is not already present under `boards/<platform>/<board_dir>/board.json`.

## 3) Metadata Authoring

- Create `boards/<platform>/<board_dir>/board.json`.
- Required fields populated:
  - `id`, `name`, `platform`, `description`, `mcu`, `flash`, `ram`, `capabilities`
- Add board taxonomy-friendly values:
  - `manufacturer`, `architecture`, `family`
- Add pin map and capability details needed for driver filtering and generation.

## 4) Validation + Smoke

- Run metadata validation:
  - `python3 tools/validate_metadata.py`
- Run board smoke generation:
  - `python3 tools/configure.py --platform <platform> --board <board_dir> --outdir tmp/board_intake/<board_dir>`
- Run completeness report:
  - `python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md`

## 5) Evidence Capture

- Save command output paths in PR description or issue comment.
- Include generated artifacts path for the smoke run.
- Add any hardware notes/runbook links if in-hand validation is done.

## 6) Done Criteria

- Metadata validation passes.
- Board-specific configure smoke passes.
- Completeness gate remains above threshold.
- Intake issue has evidence comment and is transitioned to Done.

## Intake Template

Use `docs/planning/BOARD_INTAKE_TEMPLATE.md` as the issue/PR checklist body.
