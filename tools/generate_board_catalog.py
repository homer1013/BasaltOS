#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_board(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def safe_str(value: object, default: str = "") -> str:
    text = str(value or default).strip()
    return text


def build_catalog(root: Path) -> str:
    board_files = sorted(root.glob("boards/*/*/board.json"))
    boards: list[dict] = []
    platform_counts: Counter[str] = Counter()
    manufacturer_counts: Counter[str] = Counter()

    for bf in board_files:
        data = load_board(bf)
        platform = safe_str(data.get("platform") or bf.parents[1].name, bf.parents[1].name)
        board_dir = bf.parent.name
        board_id = safe_str(data.get("id"), board_dir)
        name = safe_str(data.get("name"), board_dir)
        mcu = safe_str(data.get("mcu") or data.get("soc") or data.get("target_profile"), "unknown")
        manufacturer = safe_str(data.get("manufacturer"), "Unknown")
        capabilities = data.get("capabilities", [])
        if not isinstance(capabilities, list):
            capabilities = []

        boards.append(
            {
                "platform": platform,
                "board_dir": board_dir,
                "id": board_id,
                "name": name,
                "manufacturer": manufacturer,
                "mcu": mcu,
                "caps_count": len(capabilities),
            }
        )
        platform_counts[platform] += 1
        manufacturer_counts[manufacturer] += 1

    boards.sort(key=lambda b: (b["platform"], b["board_dir"], b["id"]))

    lines: list[str] = []
    lines.append("# Board Catalog")
    lines.append("")
    lines.append("Auto-generated from `boards/*/*/board.json` using `tools/generate_board_catalog.py`.")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Total boards: {len(boards)}")
    lines.append(f"- Platforms: {len(platform_counts)}")
    lines.append(f"- Manufacturers: {len(manufacturer_counts)}")
    lines.append("")
    lines.append("### Boards per Platform")
    lines.append("")
    for platform in sorted(platform_counts):
        lines.append(f"- {platform}: {platform_counts[platform]}")
    lines.append("")
    lines.append("### Boards per Manufacturer")
    lines.append("")
    for mfr, n in sorted(manufacturer_counts.items(), key=lambda kv: (-kv[1], kv[0].lower())):
        lines.append(f"- {mfr}: {n}")
    lines.append("")
    lines.append("## Boards")
    lines.append("")
    lines.append("| Platform | Board Dir | ID | Name | Manufacturer | MCU/SOC | Caps |")
    lines.append("|---|---|---|---|---|---|---|")
    for b in boards:
        lines.append(
            f"| {b['platform']} | {b['board_dir']} | {b['id']} | {b['name']} | "
            f"{b['manufacturer']} | {b['mcu']} | {b['caps_count']} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate deterministic board catalog markdown.")
    ap.add_argument("--out", default="docs/BOARD_CATALOG.md", help="Output markdown path")
    args = ap.parse_args()

    root = repo_root()
    out_path = (root / args.out).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    text = build_catalog(root)
    out_path.write_text(text, encoding="utf-8")
    print(f"Wrote: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
