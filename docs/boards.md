# Basalt OS board selection

Basalt supports two board-selection flows:
- `tools/board.sh` for legacy profile-copy boards (copies `sdkconfig.defaults` + `partitions.csv` into repo root)
- `tools/configure.py` for metadata-driven board/driver config generation

Board onboarding references:
- `docs/BOARD_ONBOARDING_CHECKLIST.md`
- `docs/planning/BOARD_INTAKE_TEMPLATE.md`
- `docs/BOARD_CATALOG.md` (auto-generated inventory)

Regenerate catalog after adding/changing board metadata:
```bash
python3 tools/generate_board_catalog.py
```

## Docs-First Board Addition Flow

Use this flow when adding a new board profile.

### 1) Pick Platform + Board ID

- Choose platform folder under `boards/` (example: `esp32`, `rp2040`, `atsam`).
- Choose a stable board directory and `id` (lowercase snake_case recommended).
- Planned path:
  - `boards/<platform>/<board_dir>/board.json`

### 2) Create `board.json`

Minimum required metadata:
- `id`
- `name`
- `platform`
- `description`
- `mcu` (or `soc`/`target_profile`)
- `flash`
- `ram`
- `capabilities` (non-empty list)
- `pins` (non-empty object)

Strongly recommended taxonomy fields:
- `manufacturer`
- `architecture`
- `family`

### 3) Validate Metadata

```bash
python3 tools/validate_metadata.py
```

If this fails, fix metadata first before any build attempts.

### 4) Run Configure Smoke for the New Board

```bash
python3 tools/configure.py --platform <platform> --board <board_dir> --outdir tmp/board_intake/<board_dir>
```

Expected outputs:
- `tmp/board_intake/<board_dir>/basalt_config.h`
- `tmp/board_intake/<board_dir>/basalt.features.json`
- `tmp/board_intake/<board_dir>/sdkconfig.defaults`

### 5) Check Metadata Quality Gate

```bash
python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md
```

### 6) Capture Evidence

- Save command output and output paths in the issue/PR.
- If no physical board is in hand, label as metadata-only validated.
- If in-hand hardware exists, add bench notes and mark hardware lane status.

## List boards
```bash
./tools/board.sh --list
```

Each entry is shown as:
- `profile-ready`: board includes legacy profile files
- `metadata-only`: board exists in board metadata but does not include legacy profile files yet

## Legacy profile switch (profile-ready boards)
`tools/board.sh` selector formats:
- `platform/board_dir` (recommended)
- `board_dir` (if unique)
- `board_id` from `board.json` (if unique)

Example:
```bash
./tools/board.sh esp32/cyd_3248s035r
```

Then build/flash with ESP-IDF:
```bash
# target should match board metadata; for CYD this is esp32
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configurator flow (all boards)
For metadata-only boards, use:
```bash
python tools/configure.py --platform <platform> --board <board_dir>
```

Example:
```bash
python tools/configure.py --platform esp32 --board esp32-devkitc_v4
```

This generates outputs under `config/generated/` (including `sdkconfig.defaults` used by CMake).

## Common Onboarding Pitfalls

- `platform` in JSON does not match directory platform.
- Empty `capabilities` list.
- Missing or empty `pins` object.
- Duplicate or ambiguous board IDs.
- Skipping metadata validation and debugging build errors first.

## Recommended Intake Commands (copy/paste)

```bash
python3 tools/validate_metadata.py
python3 tools/configure.py --platform <platform> --board <board_dir> --outdir tmp/board_intake/<board_dir>
python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md
```
