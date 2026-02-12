#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Dict, List, Set


def read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_boards(root: Path) -> List[Dict[str, Any]]:
    out: List[Dict[str, Any]] = []
    for board_json in sorted((root / "boards").rglob("board.json")):
        data = read_json(board_json)
        out.append(
            {
                "path": str(board_json.relative_to(root)),
                "board_dir": board_json.parent.name,
                "board_id": str(data.get("id") or board_json.parent.name),
                "platform": str(data.get("platform") or ""),
                "name": str(data.get("name") or board_json.parent.name),
                "manufacturer": str(data.get("manufacturer") or "Unknown"),
                "capabilities": {str(x).strip() for x in (data.get("capabilities") or []) if str(x).strip()},
                "pins": data.get("pins") or {},
                "display": data.get("display"),
            }
        )
    return out


def load_tft_contract(root: Path) -> Dict[str, Any]:
    mod = read_json(root / "modules" / "tft" / "module.json")
    req = mod.get("pin_requirements") or {}
    required_pins = sorted(
        [k for k, v in req.items() if isinstance(v, dict) and bool(v.get("required"))]
    )
    platforms = sorted([str(x).strip() for x in (mod.get("platforms") or []) if str(x).strip()])
    return {"required_pins": required_pins, "platforms": platforms}


def build_report(boards: List[Dict[str, Any]], tft_contract: Dict[str, Any]) -> Dict[str, Any]:
    required_pins: List[str] = list(tft_contract["required_pins"])
    tft_platforms: Set[str] = set(tft_contract["platforms"])

    boards_with_tft: List[Dict[str, Any]] = []
    spi_without_tft: List[Dict[str, Any]] = []
    display_without_tft: List[Dict[str, Any]] = []
    tft_pins_without_tft: List[Dict[str, Any]] = []
    tft_out_of_platform: List[Dict[str, Any]] = []
    tft_missing_required: List[Dict[str, Any]] = []

    for b in boards:
        caps = set(b["capabilities"])
        pins: Dict[str, Any] = dict(b["pins"])
        platform = str(b["platform"])

        has_tft = "tft" in caps
        has_spi = "spi" in caps
        has_display = b.get("display") is not None
        has_tft_pin_prefix = any(str(k).startswith("tft_") for k in pins.keys())
        missing_required = [p for p in required_pins if p not in pins]

        if has_tft:
            row = {
                "board_id": b["board_id"],
                "platform": platform,
                "manufacturer": b["manufacturer"],
                "missing_required_pins": missing_required,
                "required_pin_contract_ok": len(missing_required) == 0,
            }
            boards_with_tft.append(row)
            if missing_required:
                tft_missing_required.append(row)
            if platform not in tft_platforms:
                tft_out_of_platform.append(
                    {
                        "board_id": b["board_id"],
                        "platform": platform,
                        "manufacturer": b["manufacturer"],
                    }
                )
        else:
            if has_spi:
                spi_without_tft.append(
                    {
                        "board_id": b["board_id"],
                        "platform": platform,
                        "manufacturer": b["manufacturer"],
                    }
                )
            if has_display:
                display_without_tft.append(
                    {
                        "board_id": b["board_id"],
                        "platform": platform,
                        "manufacturer": b["manufacturer"],
                    }
                )
            if has_tft_pin_prefix:
                tft_pins_without_tft.append(
                    {
                        "board_id": b["board_id"],
                        "platform": platform,
                        "manufacturer": b["manufacturer"],
                    }
                )

    by_platform_spi = Counter(x["platform"] for x in spi_without_tft)
    by_platform_tft = Counter(x["platform"] for x in boards_with_tft)
    by_manufacturer_spi: Dict[str, int] = dict(
        sorted(Counter(x["manufacturer"] for x in spi_without_tft).items(), key=lambda kv: (-kv[1], kv[0]))
    )

    actionable_counts = {
        "display_without_tft": len(display_without_tft),
        "tft_pins_without_tft": len(tft_pins_without_tft),
        "tft_capability_missing_required_pins": len(tft_missing_required),
        "tft_capability_out_of_platform_scope": len(tft_out_of_platform),
    }
    actionable_total = sum(actionable_counts.values())

    return {
        "version": "1.0.0",
        "policy": {
            "intentional_boundary": "Boards with SPI but no TFT pin contract remain intentional external-display opt-in.",
            "actionable_boundary": "Boards with display/TFT pin posture or invalid TFT pin contracts are actionable.",
        },
        "tft_module_contract": {
            "platforms": sorted(tft_platforms),
            "required_pins": required_pins,
        },
        "summary": {
            "board_count": len(boards),
            "boards_with_tft_capability": len(boards_with_tft),
            "boards_with_spi_but_no_tft": len(spi_without_tft),
            "actionable_total": actionable_total,
            "actionable_breakdown": actionable_counts,
            "boards_with_tft_by_platform": dict(sorted(by_platform_tft.items())),
            "boards_with_spi_no_tft_by_platform": dict(sorted(by_platform_spi.items())),
            "boards_with_spi_no_tft_by_manufacturer": by_manufacturer_spi,
        },
        "lists": {
            "boards_with_tft_capability": sorted(boards_with_tft, key=lambda x: (x["platform"], x["board_id"])),
            "spi_without_tft_intentional_candidates": sorted(
                spi_without_tft, key=lambda x: (x["platform"], x["board_id"])
            ),
            "display_without_tft_actionable": sorted(
                display_without_tft, key=lambda x: (x["platform"], x["board_id"])
            ),
            "tft_pins_without_tft_actionable": sorted(
                tft_pins_without_tft, key=lambda x: (x["platform"], x["board_id"])
            ),
            "tft_missing_required_pins_actionable": sorted(
                tft_missing_required, key=lambda x: (x["platform"], x["board_id"])
            ),
            "tft_out_of_platform_actionable": sorted(
                tft_out_of_platform, key=lambda x: (x["platform"], x["board_id"])
            ),
        },
    }


def write_md(report: Dict[str, Any], out_path: Path) -> None:
    summary = report["summary"]
    actionable = report["lists"]
    lines: List[str] = []
    lines.append("# TFT Capability Posture Report v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_tft_posture_report.py`.")
    lines.append("")
    lines.append("## Policy Boundaries")
    lines.append("")
    lines.append(f"- Intentional: {report['policy']['intentional_boundary']}")
    lines.append(f"- Actionable: {report['policy']['actionable_boundary']}")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Boards analyzed: {int(summary['board_count'])}")
    lines.append(f"- Boards with `tft` capability: {int(summary['boards_with_tft_capability'])}")
    lines.append(f"- Boards with `spi` but no `tft` (intentional candidates): {int(summary['boards_with_spi_but_no_tft'])}")
    lines.append(f"- Actionable TFT posture findings: {int(summary['actionable_total'])}")
    lines.append("")
    lines.append("| Actionable Type | Count |")
    lines.append("|---|---|")
    for k, v in (summary["actionable_breakdown"] or {}).items():
        lines.append(f"| {k} | {int(v)} |")
    lines.append("")
    lines.append("## TFT Module Contract")
    lines.append("")
    lines.append(f"- Supported platforms: {', '.join(report['tft_module_contract']['platforms'])}")
    lines.append(f"- Required TFT pins: {', '.join(report['tft_module_contract']['required_pins'])}")
    lines.append("")

    if actionable["display_without_tft_actionable"]:
        lines.append("## Actionable: Display Declared Without `tft` Capability")
        lines.append("")
        for row in actionable["display_without_tft_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    if actionable["tft_pins_without_tft_actionable"]:
        lines.append("## Actionable: TFT Pin Contract Present Without `tft` Capability")
        lines.append("")
        for row in actionable["tft_pins_without_tft_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    if actionable["tft_missing_required_pins_actionable"]:
        lines.append("## Actionable: `tft` Capability Missing Required Pins")
        lines.append("")
        for row in actionable["tft_missing_required_pins_actionable"]:
            missing = ", ".join(row["missing_required_pins"])
            lines.append(f"- {row['platform']}/{row['board_id']} missing: {missing}")
        lines.append("")

    if actionable["tft_out_of_platform_actionable"]:
        lines.append("## Actionable: `tft` Capability Outside Module Platform Scope")
        lines.append("")
        for row in actionable["tft_out_of_platform_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    lines.append("## Intentional Candidate Snapshot (SPI Without `tft`, Top 20)")
    lines.append("")
    for row in actionable["spi_without_tft_intentional_candidates"][:20]:
        lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
    lines.append("")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate TFT capability posture report artifacts.")
    ap.add_argument("--json-out", default="docs/planning/TFT_CAPABILITY_POSTURE.json")
    ap.add_argument("--md-out", default="docs/planning/TFT_CAPABILITY_POSTURE.md")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()

    report = build_report(load_boards(root), load_tft_contract(root))
    json_out.parent.mkdir(parents=True, exist_ok=True)
    json_out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    write_md(report, md_out)

    print(f"[tft-posture] boards: {report['summary']['board_count']}")
    print(f"[tft-posture] tft-enabled: {report['summary']['boards_with_tft_capability']}")
    print(f"[tft-posture] spi-no-tft: {report['summary']['boards_with_spi_but_no_tft']}")
    print(f"[tft-posture] actionable: {report['summary']['actionable_total']}")
    print(f"[tft-posture] wrote: {json_out}")
    print(f"[tft-posture] wrote: {md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
