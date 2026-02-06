# Board profiles

Boards are organized as:
`boards/<platform>/<board_dir>/board.json`

Some boards also include legacy profile files:
- `sdkconfig.defaults`
- `partitions.csv`
- optional `board_config.h`

Use `tools/board.sh --list` to see which boards are:
- `profile-ready` (legacy profile files present)
- `metadata-only` (board metadata only)

For profile-ready boards, apply a profile with:
```bash
tools/board.sh <selector>
```

Selector forms:
- `platform/board_dir` (recommended)
- `board_dir` (if unique)
- `board_id` from `board.json` (if unique)

For metadata-only boards, use:
```bash
python tools/configure.py --platform <platform> --board <board_dir>
```
