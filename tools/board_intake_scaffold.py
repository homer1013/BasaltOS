#!/usr/bin/env python3
"""
Create deterministic board-intake scaffolds:

  tmp/board_intake_pipeline/<vendor_slug>/<family_slug>/<board_id>/

Artifacts:
- board.json                 starter board profile metadata
- INTAKE.md                  checklist/evidence template for intake work
- manifest.json              scaffold metadata for traceability

Optional:
- --apply writes board.json to boards/<platform>/<board_dir>/board.json
"""

from __future__ import annotations

import argparse
import json
import os
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT_ROOT = REPO_ROOT / "tmp" / "board_intake_pipeline"

PLATFORM_ARCH = {
    "esp32": "RISC-V",
    "rp2040": "ARM Cortex-M0+",
    "stm32": "ARM Cortex-M",
    "atsam": "ARM Cortex-M",
    "renesas_ra": "ARM Cortex-M",
    "renesas_rx": "Renesas RX 32-bit",
    "avr": "AVR 8-bit",
    "atmega": "AVR 8-bit",
    "pic16": "PIC 8-bit",
    "linux-sbc": "ARM Cortex-A",
}

PLATFORM_DEFAULT_CAPS = {
    "esp32": ["gpio", "uart", "i2c", "spi", "wifi"],
    "rp2040": ["gpio", "uart", "i2c", "spi", "pwm", "adc"],
    "stm32": ["gpio", "uart", "i2c", "spi", "pwm", "adc"],
    "atsam": ["gpio", "uart", "i2c", "spi", "pwm", "adc"],
    "renesas_ra": ["gpio", "uart", "i2c", "spi", "pwm", "adc"],
    "renesas_rx": ["gpio", "uart", "i2c", "spi", "pwm", "adc"],
    "avr": ["gpio", "uart", "spi", "i2c", "pwm", "adc"],
    "atmega": ["gpio", "uart", "spi", "i2c", "pwm", "adc"],
    "pic16": ["gpio", "uart", "i2c", "pwm", "adc"],
    "linux-sbc": ["gpio", "uart", "i2c", "spi", "wifi"],
}


def slugify(value: str) -> str:
    s = re.sub(r"[^a-zA-Z0-9]+", "_", str(value or "").strip().lower())
    s = re.sub(r"_+", "_", s).strip("_")
    return s


def split_csv(raw: str) -> List[str]:
    out: List[str] = []
    for x in str(raw or "").split(","):
        t = x.strip()
        if t:
            out.append(t)
    return out


def scaffold_board_json(args: argparse.Namespace) -> Dict:
    capabilities = split_csv(args.capabilities)
    if not capabilities:
        capabilities = PLATFORM_DEFAULT_CAPS.get(args.platform, ["gpio", "uart"])

    architecture = args.architecture.strip() if args.architecture else PLATFORM_ARCH.get(args.platform, "Embedded")

    return {
        "id": args.board_id,
        "name": args.board_name,
        "platform": args.platform,
        "description": args.description,
        "manufacturer": args.manufacturer,
        "architecture": architecture,
        "family": args.family,
        "mcu": args.mcu,
        "flash": args.flash,
        "ram": args.ram,
        "capabilities": capabilities,
        "pins": {
            "led": -1,
            "uart_tx": -1,
            "uart_rx": -1,
        },
        "defaults": {
            "modules": ["uart", "shell_min"],
            "applets": [],
            "market_apps": [],
        },
    }


def intake_markdown(args: argparse.Namespace, rel_board_json: str) -> str:
    return f"""# Board Intake: {args.board_name}

## Identity

- Manufacturer: {args.manufacturer}
- Family: {args.family}
- Platform: {args.platform}
- Board id: {args.board_id}
- Board dir: {args.board_dir}

## Scaffold Paths

- Working scaffold root: `{args.output_root}`
- Starter board profile: `{rel_board_json}`

## Next Steps

1. Fill final pin map in `board.json`.
2. Verify capabilities/default modules align with hardware reality.
3. Run validation commands:
   - `python3 tools/validate_metadata.py`
   - `python3 tools/configure.py --platform {args.platform} --board {args.board_dir} --outdir tmp/board_intake/{args.board_dir}`
   - `python3 tools/metadata_completeness_report.py --fail-under 95 --out tmp/metadata/board_completeness_report.md`
4. Capture evidence and bench notes in Jira.

## Source Docs

- Product page:
- Pinout:
- Datasheet:
"""


def write_json(path: Path, obj: Dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Scaffold a deterministic board intake workspace.")
    ap.add_argument("--manufacturer", required=True, help="Manufacturer display name, e.g. 'Adafruit'")
    ap.add_argument("--family", required=True, help="Product family, e.g. 'SAMD21'")
    ap.add_argument("--platform", required=True, help="Basalt platform id, e.g. esp32/rp2040/atsam")
    ap.add_argument("--board-id", required=True, help="Board id for board.json (slug form recommended)")
    ap.add_argument("--board-name", required=True, help="Human name, e.g. 'Circuit Playground Express'")
    ap.add_argument("--board-dir", default="", help="Board directory name under boards/<platform>; defaults to --board-id")
    ap.add_argument("--mcu", default="UNKNOWN_MCU", help="MCU/SOC field")
    ap.add_argument("--flash", default="unknown", help="Flash size text")
    ap.add_argument("--ram", default="unknown", help="RAM size text")
    ap.add_argument("--description", default="Board intake scaffold profile", help="Board description")
    ap.add_argument("--architecture", default="", help="Optional architecture override")
    ap.add_argument("--capabilities", default="", help="CSV capability list; defaults per platform")
    ap.add_argument("--output-root", default=str(DEFAULT_OUT_ROOT), help="Root output folder")
    ap.add_argument("--apply", action="store_true", help="Write board.json to boards/<platform>/<board_dir>/board.json")
    ap.add_argument("--force", action="store_true", help="Allow overwrite of existing target board.json on --apply")
    args = ap.parse_args()

    args.board_id = slugify(args.board_id)
    if not args.board_id:
        raise SystemExit("error: --board-id becomes empty after slug normalization")
    args.board_dir = slugify(args.board_dir) if args.board_dir else args.board_id
    if not args.board_dir:
        raise SystemExit("error: --board-dir becomes empty after slug normalization")

    vendor_slug = slugify(args.manufacturer)
    family_slug = slugify(args.family)
    if not vendor_slug or not family_slug:
        raise SystemExit("error: --manufacturer/--family must not be empty")

    out_root = Path(args.output_root)
    scaffold_root = out_root / vendor_slug / family_slug / args.board_id
    board_json_path = scaffold_root / "board.json"
    intake_md_path = scaffold_root / "INTAKE.md"
    manifest_path = scaffold_root / "manifest.json"

    board_obj = scaffold_board_json(args)
    write_json(board_json_path, board_obj)
    rel_board_json = os.path.relpath(board_json_path.resolve(), REPO_ROOT.resolve())
    intake_md_path.write_text(intake_markdown(args, rel_board_json), encoding="utf-8")
    manifest = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "manufacturer": args.manufacturer,
        "family": args.family,
        "vendor_slug": vendor_slug,
        "family_slug": family_slug,
        "board_id": args.board_id,
        "board_dir": args.board_dir,
        "platform": args.platform,
        "scaffold_root": str(scaffold_root),
    }
    write_json(manifest_path, manifest)

    print(f"[intake] scaffold root: {scaffold_root}")
    print(f"[intake] wrote: {board_json_path}")
    print(f"[intake] wrote: {intake_md_path}")
    print(f"[intake] wrote: {manifest_path}")

    if args.apply:
        target = REPO_ROOT / "boards" / args.platform / args.board_dir / "board.json"
        if target.exists() and not args.force:
            raise SystemExit(f"error: target already exists: {target} (use --force to overwrite)")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(board_json_path.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"[intake] applied board profile: {target}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
