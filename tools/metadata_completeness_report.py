#!/usr/bin/env python3
"""
Generate board metadata completeness report for BasaltOS boards.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


REQUIRED_FIELDS = [
    "id",
    "name",
    "platform",
    "description",
    "mcu",
    "flash",
    "ram",
    "capabilities",
]


def load_board_files(boards_root: Path) -> list[Path]:
    return sorted(boards_root.rglob("board.json"))


def as_nonempty(value: Any) -> bool:
    if value is None:
        return False
    if isinstance(value, str):
        return bool(value.strip())
    if isinstance(value, (list, dict)):
        return len(value) > 0
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate board metadata completeness report")
    ap.add_argument("--boards-root", default="boards", help="Boards root directory")
    ap.add_argument(
        "--out",
        default="tmp/metadata/board_completeness_report.md",
        help="Output markdown file",
    )
    ap.add_argument(
        "--fail-under",
        type=float,
        default=None,
        help="Fail (exit 2) when completeness percentage is below this threshold",
    )
    args = ap.parse_args()

    boards_root = Path(args.boards_root)
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)

    files = load_board_files(boards_root)
    if not files:
        raise SystemExit(f"no board.json files under {boards_root}")

    missing_by_field = {k: 0 for k in REQUIRED_FIELDS}
    rows: list[tuple[str, int, int]] = []

    for f in files:
        try:
            d = json.loads(f.read_text(encoding="utf-8"))
        except Exception:
            rows.append((str(f), len(REQUIRED_FIELDS), len(REQUIRED_FIELDS)))
            for k in REQUIRED_FIELDS:
                missing_by_field[k] += 1
            continue
        missing = [k for k in REQUIRED_FIELDS if not as_nonempty(d.get(k))]
        rows.append((str(f), len(REQUIRED_FIELDS), len(missing)))
        for k in missing:
            missing_by_field[k] += 1

    total = len(files)
    overall_missing = sum(x[2] for x in rows)
    overall_slots = total * len(REQUIRED_FIELDS)
    pct = 100.0 * (overall_slots - overall_missing) / overall_slots if overall_slots else 0.0

    lines = []
    lines.append("# Board Metadata Completeness Report")
    lines.append("")
    lines.append(f"- Boards scanned: {total}")
    lines.append(f"- Required fields per board: {len(REQUIRED_FIELDS)}")
    lines.append(f"- Overall completeness: {pct:.2f}%")
    lines.append("")
    lines.append("## Missing By Field")
    lines.append("")
    for k in REQUIRED_FIELDS:
        lines.append(f"- `{k}` missing on {missing_by_field[k]} board(s)")
    lines.append("")
    lines.append("## Board Gaps")
    lines.append("")
    lines.append("| Board File | Missing Fields |")
    lines.append("|---|---|")
    for path, _slots, miss in sorted(rows, key=lambda r: (-r[2], r[0])):
        lines.append(f"| `{path}` | {miss} |")

    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[metadata] wrote: {out.resolve()}")
    print(f"[metadata] boards: {total}")
    print(f"[metadata] completeness_pct: {pct:.2f}")

    if args.fail_under is not None:
        threshold = float(args.fail_under)
        if pct < threshold:
            print(f"[metadata] gate: FAIL (completeness {pct:.2f}% < threshold {threshold:.2f}%)")
            return 2
        print(f"[metadata] gate: PASS (completeness {pct:.2f}% >= threshold {threshold:.2f}%)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
