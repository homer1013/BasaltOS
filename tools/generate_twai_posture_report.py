#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path
from typing import Any, Dict, List, Set


def read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_boards(root: Path) -> List[Dict[str, Any]]:
    out: List[Dict[str, Any]] = []
    for board_json in sorted((root / "boards").rglob("board.json")):
        data = read_json(board_json)
        pins = data.get("pins") or {}
        out.append(
            {
                "board_id": str(data.get("id") or board_json.parent.name),
                "platform": str(data.get("platform") or ""),
                "manufacturer": str(data.get("manufacturer") or "Unknown"),
                "capabilities": {str(x).strip() for x in (data.get("capabilities") or []) if str(x).strip()},
                "has_can_pin_contract": ("can_tx" in pins and "can_rx" in pins),
                "path": str(board_json.relative_to(root)),
            }
        )
    return out


def load_twai_contract(root: Path) -> Dict[str, Any]:
    mod = read_json(root / "modules" / "twai" / "module.json")
    req = mod.get("pins") or []
    return {
        "platforms": sorted([str(x).strip() for x in (mod.get("platforms") or []) if str(x).strip()]),
        "required_pins": [str(x).strip() for x in req if str(x).strip()],
    }


def build_report(boards: List[Dict[str, Any]], contract: Dict[str, Any]) -> Dict[str, Any]:
    supported_platforms: Set[str] = set(contract["platforms"])

    twai_enabled: List[Dict[str, Any]] = []
    esp32_missing_twai_with_can_pins: List[Dict[str, Any]] = []
    esp32_missing_twai_no_can_pins: List[Dict[str, Any]] = []
    twai_enabled_no_can_pins: List[Dict[str, Any]] = []
    twai_enabled_out_of_platform: List[Dict[str, Any]] = []
    non_esp32_missing_twai: List[Dict[str, Any]] = []

    for b in boards:
        has_twai = "twai" in b["capabilities"]
        is_esp32 = b["platform"] == "esp32"
        has_can_pins = bool(b["has_can_pin_contract"])

        base = {
            "board_id": b["board_id"],
            "platform": b["platform"],
            "manufacturer": b["manufacturer"],
        }

        if has_twai:
            twai_enabled.append(base)
            if not has_can_pins:
                twai_enabled_no_can_pins.append(base)
            if b["platform"] not in supported_platforms:
                twai_enabled_out_of_platform.append(base)
            continue

        if is_esp32:
            if has_can_pins:
                esp32_missing_twai_with_can_pins.append(base)
            else:
                esp32_missing_twai_no_can_pins.append(base)
        else:
            non_esp32_missing_twai.append(base)

    actionable_total = (
        len(esp32_missing_twai_with_can_pins)
        + len(twai_enabled_no_can_pins)
        + len(twai_enabled_out_of_platform)
    )

    return {
        "version": "1.0.0",
        "policy": {
            "intentional_boundary": "Non-ESP32 boards and ESP32 boards without CAN pin contract remain intentional TWAI opt-in backlog.",
            "actionable_boundary": "ESP32 boards with CAN pin contract but missing twai capability, or invalid twai capability contract, are actionable.",
        },
        "twai_module_contract": contract,
        "summary": {
            "board_count": len(boards),
            "twai_enabled_count": len(twai_enabled),
            "esp32_missing_twai_with_can_pin_contract": len(esp32_missing_twai_with_can_pins),
            "esp32_missing_twai_without_can_pin_contract": len(esp32_missing_twai_no_can_pins),
            "non_esp32_missing_twai": len(non_esp32_missing_twai),
            "actionable_total": actionable_total,
            "twai_enabled_by_platform": dict(sorted(Counter(x["platform"] for x in twai_enabled).items())),
            "non_esp32_missing_twai_by_platform": dict(
                sorted(Counter(x["platform"] for x in non_esp32_missing_twai).items())
            ),
        },
        "lists": {
            "twai_enabled": sorted(twai_enabled, key=lambda x: (x["platform"], x["board_id"])),
            "esp32_missing_twai_with_can_pin_contract_actionable": sorted(
                esp32_missing_twai_with_can_pins, key=lambda x: (x["platform"], x["board_id"])
            ),
            "esp32_missing_twai_without_can_pin_contract_intentional": sorted(
                esp32_missing_twai_no_can_pins, key=lambda x: (x["platform"], x["board_id"])
            ),
            "non_esp32_missing_twai_intentional": sorted(
                non_esp32_missing_twai, key=lambda x: (x["platform"], x["board_id"])
            ),
            "twai_enabled_missing_can_pin_contract_actionable": sorted(
                twai_enabled_no_can_pins, key=lambda x: (x["platform"], x["board_id"])
            ),
            "twai_enabled_out_of_platform_actionable": sorted(
                twai_enabled_out_of_platform, key=lambda x: (x["platform"], x["board_id"])
            ),
        },
    }


def write_md(report: Dict[str, Any], out_path: Path) -> None:
    s = report["summary"]
    lines: List[str] = []
    lines.append("# TWAI Capability Posture Report v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_twai_posture_report.py`.")
    lines.append("")
    lines.append("## Policy Boundaries")
    lines.append("")
    lines.append(f"- Intentional: {report['policy']['intentional_boundary']}")
    lines.append(f"- Actionable: {report['policy']['actionable_boundary']}")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Boards analyzed: {int(s['board_count'])}")
    lines.append(f"- Boards with `twai` capability: {int(s['twai_enabled_count'])}")
    lines.append(
        f"- ESP32 missing `twai` with CAN pin contract (actionable): {int(s['esp32_missing_twai_with_can_pin_contract'])}"
    )
    lines.append(
        f"- ESP32 missing `twai` without CAN pin contract (intentional): {int(s['esp32_missing_twai_without_can_pin_contract'])}"
    )
    lines.append(f"- Non-ESP32 missing `twai` (intentional): {int(s['non_esp32_missing_twai'])}")
    lines.append(f"- Actionable total: {int(s['actionable_total'])}")
    lines.append("")

    actionable = report["lists"]
    if actionable["esp32_missing_twai_with_can_pin_contract_actionable"]:
        lines.append("## Actionable: ESP32 Boards With CAN Pin Contract Missing `twai`")
        lines.append("")
        for row in actionable["esp32_missing_twai_with_can_pin_contract_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    if actionable["twai_enabled_missing_can_pin_contract_actionable"]:
        lines.append("## Actionable: `twai` Enabled But CAN Pin Contract Missing")
        lines.append("")
        for row in actionable["twai_enabled_missing_can_pin_contract_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    if actionable["twai_enabled_out_of_platform_actionable"]:
        lines.append("## Actionable: `twai` Enabled Outside Module Platform Scope")
        lines.append("")
        for row in actionable["twai_enabled_out_of_platform_actionable"]:
            lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
        lines.append("")

    lines.append("## Intentional Queue Snapshot: Non-ESP32 Missing `twai` (Top 20)")
    lines.append("")
    for row in actionable["non_esp32_missing_twai_intentional"][:20]:
        lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
    lines.append("")

    lines.append("## Intentional Queue Snapshot: ESP32 Missing `twai` Without CAN Pins")
    lines.append("")
    for row in actionable["esp32_missing_twai_without_can_pin_contract_intentional"]:
        lines.append(f"- {row['platform']}/{row['board_id']} ({row['manufacturer']})")
    lines.append("")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate TWAI capability posture report artifacts.")
    ap.add_argument("--json-out", default="docs/planning/TWAI_CAPABILITY_POSTURE.json")
    ap.add_argument("--md-out", default="docs/planning/TWAI_CAPABILITY_POSTURE.md")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    report = build_report(load_boards(root), load_twai_contract(root))

    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()
    json_out.parent.mkdir(parents=True, exist_ok=True)
    json_out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    write_md(report, md_out)

    print(f"[twai-posture] boards: {report['summary']['board_count']}")
    print(f"[twai-posture] twai-enabled: {report['summary']['twai_enabled_count']}")
    print(f"[twai-posture] actionable: {report['summary']['actionable_total']}")
    print(f"[twai-posture] wrote: {json_out}")
    print(f"[twai-posture] wrote: {md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
