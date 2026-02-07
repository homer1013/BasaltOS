# Basalt OS board selection

Basalt supports two board-selection flows:
- `tools/board.sh` for legacy profile-copy boards (copies `sdkconfig.defaults` + `partitions.csv` into repo root)
- `tools/configure.py` for metadata-driven board/driver config generation

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
