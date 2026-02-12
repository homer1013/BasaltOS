#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple


PLATFORM_ALIAS = {
    "atmega": {"atmega", "avr"},
    "avr": {"avr", "atmega"},
    "pic16": {"pic16", "pic"},
    "pic": {"pic", "pic16"},
}


def read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def discover_modules(root: Path) -> Dict[str, Dict[str, Any]]:
    out: Dict[str, Dict[str, Any]] = {}
    for module_json in sorted((root / "modules").rglob("module.json")):
        data = read_json(module_json)
        mid = str(data.get("id") or "").strip()
        if not mid:
            continue
        out[mid] = data
    return out


def discover_boards(root: Path) -> List[Dict[str, Any]]:
    rows: List[Dict[str, Any]] = []
    boards_root = root / "boards"
    for board_json in sorted(boards_root.rglob("board.json")):
        data = read_json(board_json)
        platform = str(data.get("platform") or "").strip()
        board_dir = board_json.parent.name
        board_id = str(data.get("id") or board_dir).strip()
        rows.append(
            {
                "platform": platform,
                "board_dir": board_dir,
                "board_id": board_id,
                "name": str(data.get("name") or board_id),
                "data": data,
            }
        )
    rows.sort(key=lambda r: (r["platform"], r["board_dir"]))
    return rows


def board_capabilities(board_data: Dict[str, Any]) -> Optional[Set[str]]:
    caps = board_data.get("capabilities")
    if not isinstance(caps, list):
        return None
    out = {str(c).strip() for c in caps if str(c).strip()}
    out.add("fs_spiffs")
    if "sd_card" in out:
        out.add("fs_sd")
    if "uart" in out:
        out.update({"shell_full", "shell_min", "shell_tiny"})
    if "i2c" in out:
        out.update({"rtc", "imu", "display_ssd1306", "ads1115", "bme280"})
    if "spi" in out:
        out.add("mcp2515")
    if "gpio" in out:
        out.update({"dht22", "uln2003", "l298n"})
    if "adc" in out or "i2s" in out:
        out.add("mic")
    return out


def module_platform_match(board_platform: str, module_platforms: List[str]) -> bool:
    if not module_platforms:
        return True
    board_aliases = PLATFORM_ALIAS.get(board_platform, {board_platform})
    module_set = {str(x).strip() for x in module_platforms if str(x).strip()}
    return any(alias in module_set for alias in board_aliases)


def resolve_single_module(
    modules: Dict[str, Dict[str, Any]], module_id: str, allow: Optional[Set[str]]
) -> Tuple[bool, str]:
    if module_id not in modules:
        return False, "unknown_module"

    enabled: Set[str] = set()
    stack: List[str] = [module_id]

    while stack:
        mid = stack.pop()
        if mid in enabled:
            continue
        if mid not in modules:
            return False, f"missing_module:{mid}"
        if allow is not None and mid not in allow:
            return False, f"not_allowed:{mid}"

        enabled.add(mid)
        for dep in [str(x).strip() for x in (modules[mid].get("depends") or []) if str(x).strip()]:
            if dep not in modules:
                return False, f"missing_dependency:{dep}"
            if allow is not None and dep not in allow:
                return False, f"dependency_not_allowed:{dep}"
            if dep not in enabled:
                stack.append(dep)

    for mid in enabled:
        for conf in [str(x).strip() for x in (modules[mid].get("conflicts") or []) if str(x).strip()]:
            if {mid, conf} == {"fs_spiffs", "fs_sd"}:
                continue
            if conf in enabled:
                return False, f"conflict:{mid}<->{conf}"

    return True, "ok"


def build_rows(modules: Dict[str, Dict[str, Any]], boards: List[Dict[str, Any]]) -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    module_ids = sorted(modules.keys())

    for b in boards:
        platform = str(b["platform"])
        allow = board_capabilities(b["data"])
        for mid in module_ids:
            m = modules[mid]
            m_platforms = [str(x).strip() for x in (m.get("platforms") or []) if str(x).strip()]
            plat_match = module_platform_match(platform, m_platforms)
            allow_contains = allow is None or (mid in allow)
            resolved, resolve_reason = resolve_single_module(modules, mid, allow)

            if resolved and plat_match:
                status = "SUPPORTED"
                reason = "-"
            elif resolved and not plat_match:
                status = "CAPABILITY_ONLY"
                reason = "module_platform_mismatch"
            else:
                status = "UNSUPPORTED"
                reason = resolve_reason

            rows.append(
                {
                    "platform": platform,
                    "board_dir": b["board_dir"],
                    "board_id": b["board_id"],
                    "board_name": b["name"],
                    "module_id": mid,
                    "module_name": str(m.get("name") or mid),
                    "module_platforms": ",".join(m_platforms),
                    "module_platform_match": "yes" if plat_match else "no",
                    "allow_contains_module": "yes" if allow_contains else "no",
                    "resolve_with_allow": "yes" if resolved else "no",
                    "status": status,
                    "reason": reason,
                }
            )

    rows.sort(key=lambda r: (r["platform"], r["board_dir"], r["module_id"]))
    return rows


def build_summary(rows: List[Dict[str, str]]) -> Dict[str, Any]:
    status_counts = Counter(r["status"] for r in rows)
    by_platform: Dict[str, Dict[str, int]] = defaultdict(lambda: {"boards": 0, "SUPPORTED": 0, "CAPABILITY_ONLY": 0, "UNSUPPORTED": 0})
    boards_seen: Dict[str, Set[str]] = defaultdict(set)
    module_supported = Counter()

    for r in rows:
        p = r["platform"]
        b = r["board_dir"]
        boards_seen[p].add(b)
        by_platform[p][r["status"]] += 1
        if r["status"] == "SUPPORTED":
            module_supported[r["module_id"]] += 1

    for p in by_platform.keys():
        by_platform[p]["boards"] = len(boards_seen[p])

    module_coverage = [
        {"module_id": mid, "supported_boards": int(cnt)}
        for mid, cnt in sorted(module_supported.items(), key=lambda kv: (-kv[1], kv[0]))
    ]

    return {
        "status_counts": dict(status_counts),
        "platform_summary": {k: by_platform[k] for k in sorted(by_platform.keys())},
        "module_coverage": module_coverage,
    }


def write_csv(rows: List[Dict[str, str]], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "platform",
        "board_dir",
        "board_id",
        "board_name",
        "module_id",
        "module_name",
        "module_platforms",
        "module_platform_match",
        "allow_contains_module",
        "resolve_with_allow",
        "status",
        "reason",
    ]
    with out_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow(r)


def write_json(rows: List[Dict[str, str]], summary: Dict[str, Any], out_path: Path, root: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "version": "1.0.0",
        "generator": "tools/generate_driver_capability_matrix.py",
        "row_count": len(rows),
        "summary": summary,
        "rows": rows,
    }
    out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def write_md(summary: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    st = summary.get("status_counts") or {}
    lines: List[str] = []
    lines.append("# Driver Capability Matrix v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_driver_capability_matrix.py`.")
    lines.append("")
    lines.append(f"- SUPPORTED rows: {int(st.get('SUPPORTED', 0))}")
    lines.append(f"- CAPABILITY_ONLY rows: {int(st.get('CAPABILITY_ONLY', 0))}")
    lines.append(f"- UNSUPPORTED rows: {int(st.get('UNSUPPORTED', 0))}")
    lines.append("")
    lines.append("## Platform Summary")
    lines.append("")
    lines.append("| Platform | Boards | Supported | Capability Only | Unsupported |")
    lines.append("|---|---|---|---|---|")
    for p, row in (summary.get("platform_summary") or {}).items():
        lines.append(
            f"| {p} | {int(row.get('boards', 0))} | {int(row.get('SUPPORTED', 0))} | {int(row.get('CAPABILITY_ONLY', 0))} | {int(row.get('UNSUPPORTED', 0))} |"
        )
    lines.append("")
    lines.append("## Module Coverage (Top 20)")
    lines.append("")
    lines.append("| Module | Boards Supported |")
    lines.append("|---|---|")
    for item in (summary.get("module_coverage") or [])[:20]:
        lines.append(f"| {item['module_id']} | {int(item['supported_boards'])} |")
    lines.append("")
    lines.append("Artifacts:")
    lines.append("- `docs/planning/DRIVER_CAPABILITY_MATRIX.csv`")
    lines.append("- `docs/planning/DRIVER_CAPABILITY_MATRIX.json`")
    lines.append("")
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate board x driver capability matrix artifacts.")
    ap.add_argument("--csv-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.csv", help="CSV output path")
    ap.add_argument("--json-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.json", help="JSON output path")
    ap.add_argument("--md-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.md", help="Markdown output path")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    csv_out = (root / args.csv_out).resolve()
    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()

    modules = discover_modules(root)
    boards = discover_boards(root)
    rows = build_rows(modules, boards)
    summary = build_summary(rows)

    write_csv(rows, csv_out)
    write_json(rows, summary, json_out, root)
    write_md(summary, md_out)

    print(f"[driver-matrix] boards: {len(boards)}")
    print(f"[driver-matrix] modules: {len(modules)}")
    print(f"[driver-matrix] rows: {len(rows)}")
    print(f"[driver-matrix] wrote: {csv_out}")
    print(f"[driver-matrix] wrote: {json_out}")
    print(f"[driver-matrix] wrote: {md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
