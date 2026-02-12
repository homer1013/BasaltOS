#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
import glob
import json
import re
import sys
from pathlib import Path

invalid_tokens = {"", "unknown", "tbd", "todo", "n/a", "na"}
id_re = re.compile(r"^[a-z0-9_-]+$")

idx_path = Path("docs/BOARD_TAXONOMY_INDEX.json")
if not idx_path.exists():
    raise SystemExit("FAIL: missing docs/BOARD_TAXONOMY_INDEX.json")

idx = json.loads(idx_path.read_text(encoding="utf-8"))
rows = idx.get("boards") or []
if not isinstance(rows, list) or not rows:
    raise SystemExit("FAIL: taxonomy index has no board rows")

summary = idx.get("summary") or {}
if int(summary.get("total_boards", -1)) != len(rows):
    raise SystemExit("FAIL: taxonomy summary total_boards mismatch")

row_map = {}
ids = set()
errors = []

for i, row in enumerate(rows, 1):
    if not isinstance(row, dict):
        errors.append(f"index row {i}: expected object")
        continue

    platform = str(row.get("platform", "")).strip()
    board_dir = str(row.get("board_dir", "")).strip()
    board_id = str(row.get("id", "")).strip()
    manufacturer = str(row.get("manufacturer", "")).strip()
    architecture = str(row.get("architecture", "")).strip()
    family = str(row.get("family", "")).strip()

    if not platform or not board_dir:
        errors.append(f"index row {i}: missing platform/board_dir")
        continue

    key = (platform, board_dir)
    if key in row_map:
        errors.append(f"index row {i}: duplicate platform/board_dir {platform}/{board_dir}")
    row_map[key] = row

    if not board_id or not id_re.match(board_id):
        errors.append(f"index row {i}: invalid id '{board_id}'")
    elif board_id in ids:
        errors.append(f"index row {i}: duplicate id '{board_id}'")
    ids.add(board_id)

    for label, value in (("manufacturer", manufacturer), ("architecture", architecture), ("family", family)):
        if not value or value.strip().lower() in invalid_tokens:
            errors.append(f"index row {i}: invalid {label} '{value}'")

files = sorted(glob.glob("boards/*/*/board.json"))
if not files:
    raise SystemExit("FAIL: no board.json files found")

for bf in files:
    p = Path(bf)
    platform = p.parts[1]
    board_dir = p.parts[2]
    key = (platform, board_dir)
    row = row_map.get(key)
    if row is None:
        errors.append(f"{bf}: missing in taxonomy index")
        continue

    data = json.loads(p.read_text(encoding="utf-8"))
    board_id = str(data.get("id", "")).strip()
    if not board_id or not id_re.match(board_id):
        errors.append(f"{bf}: invalid id '{board_id}'")

    for field in ("manufacturer", "architecture", "family"):
        value = str(data.get(field, "")).strip()
        if not value or value.strip().lower() in invalid_tokens:
            errors.append(f"{bf}: invalid {field} '{value}'")
        if str(row.get(field, "")).strip() != value:
            errors.append(
                f"{bf}: taxonomy drift for {field} (board.json='{value}' index='{row.get(field, '')}')"
            )

    if str(row.get("id", "")).strip() != board_id:
        errors.append(
            f"{bf}: taxonomy drift for id (board.json='{board_id}' index='{row.get('id', '')}')"
        )

if len(row_map) != len(files):
    extras = sorted(set(row_map.keys()) - {(Path(bf).parts[1], Path(bf).parts[2]) for bf in files})
    if extras:
        errors.append(f"taxonomy index contains {len(extras)} stale rows (example: {extras[0][0]}/{extras[0][1]})")

if errors:
    print("FAIL: board taxonomy schema smoke checks failed:")
    for e in errors[:120]:
        print(f"- {e}")
    if len(errors) > 120:
        print(f"- ... and {len(errors)-120} more")
    sys.exit(1)

print("PASS: board taxonomy schema smoke checks passed")
PY
