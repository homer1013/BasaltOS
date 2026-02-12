# Board Intake Template

Copy this into a Jira issue or PR description for new board onboarding.

## Identity

- Manufacturer:
- Board name:
- Board id (`board.json id`):
- Platform:
- Target profile:

## Source Docs

- Product page:
- Pinout reference:
- Datasheet:

## Metadata Path

- `boards/<platform>/<board_dir>/board.json`:

## Required Fields Check

- [ ] `id`
- [ ] `name`
- [ ] `platform`
- [ ] `description`
- [ ] `mcu`
- [ ] `flash`
- [ ] `ram`
- [ ] `capabilities`

## Taxonomy Fields

- [ ] `manufacturer`
- [ ] `architecture`
- [ ] `family`
- [ ] `silicon` is derivable from `mcu` or explicit fielding

## Validation Commands

```bash
python3 tools/validate_metadata.py
python3 tools/configure.py --platform <platform> --board <board_dir> --outdir tmp/board_intake/<board_dir>
python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md
```

## Evidence

- Validation output:
- Configure smoke output:
- Completeness gate output:
- Hardware notes (optional):

## Outcome

- [ ] Done
- [ ] Blocked (reason + owner + unblock date)
