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
    # Policy: timer is treated as a baseline runtime capability for all boards.
    # Many modules (for example pwm/dht22) depend on timer services even when
    # board metadata omitted an explicit "timer" capability entry.
    out.add("timer")
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


def build_gap_report(rows: List[Dict[str, str]]) -> Dict[str, Any]:
    by_platform: Dict[str, Dict[str, Any]] = {}
    for r in rows:
        if r.get("status") == "SUPPORTED":
            continue
        platform = str(r.get("platform") or "")
        module_id = str(r.get("module_id") or "")
        reason = str(r.get("reason") or "")
        if platform not in by_platform:
            by_platform[platform] = {
                "unsupported_or_gap_rows": 0,
                "module_gap_counts": Counter(),
                "reason_counts": Counter(),
            }
        by_platform[platform]["unsupported_or_gap_rows"] += 1
        by_platform[platform]["module_gap_counts"][module_id] += 1
        by_platform[platform]["reason_counts"][reason] += 1

    out: Dict[str, Any] = {}
    for platform, data in sorted(by_platform.items()):
        module_top = [
            {"module_id": mid, "count": int(cnt)}
            for mid, cnt in data["module_gap_counts"].most_common(10)
        ]
        reason_top = [
            {"reason": reason, "count": int(cnt)}
            for reason, cnt in data["reason_counts"].most_common(8)
        ]
        out[platform] = {
            "unsupported_or_gap_rows": int(data["unsupported_or_gap_rows"]),
            "top_module_gaps": module_top,
            "top_reasons": reason_top,
        }

    return {
        "version": "1.0.0",
        "platform_count": len(out),
        "platforms": out,
    }


def classify_mismatch_policy(row: Dict[str, str]) -> Dict[str, str]:
    platform = str(row.get("platform") or "")
    module_id = str(row.get("module_id") or "")
    reason = str(row.get("reason") or "")

    if reason != "module_platform_mismatch":
        return {"bucket": "non_mismatch", "label": "non_mismatch"}

    if platform in {"atmega", "avr", "pic16"} and module_id == "mic":
        return {"bucket": "intentional", "label": "intentional_8bit_mic_scope"}

    if module_id in {"wifi", "bluetooth"} and platform != "esp32":
        return {"bucket": "intentional", "label": "intentional_wireless_scope"}

    if platform == "linux-sbc":
        return {"bucket": "actionable", "label": "actionable_linux_sbc_platform_alignment"}

    return {"bucket": "actionable", "label": "actionable_platform_scope_review"}


def build_policy_summary(rows: List[Dict[str, str]]) -> Dict[str, Any]:
    mismatch_rows = [r for r in rows if str(r.get("reason") or "") == "module_platform_mismatch"]
    bucket_counts = Counter()
    label_counts = Counter()
    actionable_by_platform: Dict[str, Counter] = defaultdict(Counter)

    for r in mismatch_rows:
        c = classify_mismatch_policy(r)
        bucket = c["bucket"]
        label = c["label"]
        bucket_counts[bucket] += 1
        label_counts[label] += 1
        if bucket == "actionable":
            actionable_by_platform[str(r.get("platform") or "")][str(r.get("module_id") or "")] += 1

    actionable_top: Dict[str, List[Dict[str, Any]]] = {}
    for platform in sorted(actionable_by_platform.keys()):
        actionable_top[platform] = [
            {"module_id": mid, "count": int(cnt)}
            for mid, cnt in actionable_by_platform[platform].most_common(8)
        ]

    return {
        "mismatch_row_count": len(mismatch_rows),
        "bucket_counts": dict(bucket_counts),
        "label_counts": dict(label_counts),
        "actionable_top_modules_by_platform": actionable_top,
    }


ADVANCED_PERIPHERAL_MODULES = {
    "eeprom",
    "fs_sd",
    "hp4067",
    "mcp23017",
    "mcp2544fd",
    "psram",
    "remote_node",
    "tft",
    "tp4056",
    "twai",
}


def classify_not_allowed_policy(row: Dict[str, str]) -> Dict[str, str]:
    platform = str(row.get("platform") or "")
    reason = str(row.get("reason") or "")
    if not reason.startswith("not_allowed:"):
        return {"bucket": "non_not_allowed", "label": "non_not_allowed", "module": ""}

    blocked_module = reason.split(":", 1)[1].strip()

    if blocked_module in ADVANCED_PERIPHERAL_MODULES:
        return {
            "bucket": "intentional",
            "label": "intentional_advanced_peripheral_opt_in",
            "module": blocked_module,
        }

    if blocked_module in {"wifi", "bluetooth"} and platform != "esp32":
        return {
            "bucket": "intentional",
            "label": "intentional_wireless_capability_gate",
            "module": blocked_module,
        }

    if blocked_module in {"adc", "gpio", "dht22", "pwm", "mic", "l298n", "uln2003"}:
        return {
            "bucket": "actionable",
            "label": "actionable_foundation_capability_gap",
            "module": blocked_module,
        }

    return {
        "bucket": "actionable",
        "label": "actionable_capability_scope_review",
        "module": blocked_module,
    }


def build_advanced_policy_summary(rows: List[Dict[str, str]]) -> Dict[str, Any]:
    not_allowed_rows = [r for r in rows if str(r.get("reason") or "").startswith("not_allowed:")]
    bucket_counts = Counter()
    label_counts = Counter()
    actionable_by_platform: Dict[str, Counter] = defaultdict(Counter)

    for r in not_allowed_rows:
        c = classify_not_allowed_policy(r)
        bucket = c["bucket"]
        label = c["label"]
        blocked_module = c["module"]
        bucket_counts[bucket] += 1
        label_counts[label] += 1
        if bucket == "actionable" and blocked_module:
            actionable_by_platform[str(r.get("platform") or "")][blocked_module] += 1

    actionable_top: Dict[str, List[Dict[str, Any]]] = {}
    for platform in sorted(actionable_by_platform.keys()):
        actionable_top[platform] = [
            {"module_id": mid, "count": int(cnt)}
            for mid, cnt in actionable_by_platform[platform].most_common(8)
        ]

    return {
        "not_allowed_row_count": len(not_allowed_rows),
        "bucket_counts": dict(bucket_counts),
        "label_counts": dict(label_counts),
        "actionable_top_modules_by_platform": actionable_top,
    }


def classify_row_policy(row: Dict[str, str]) -> Dict[str, str]:
    reason = str(row.get("reason") or "")
    if reason == "module_platform_mismatch":
        c = classify_mismatch_policy(row)
        return {"bucket": c["bucket"], "label": c["label"], "module": str(row.get("module_id") or "")}
    if reason.startswith("not_allowed:"):
        return classify_not_allowed_policy(row)
    return {"bucket": "unclassified", "label": "unclassified_reason", "module": str(row.get("module_id") or "")}


def build_foundation_focus_summary(rows: List[Dict[str, str]]) -> Dict[str, Any]:
    gap_rows = [r for r in rows if str(r.get("status") or "") != "SUPPORTED"]
    bucket_counts = Counter()
    label_counts = Counter()
    actionable_by_platform: Dict[str, Counter] = defaultdict(Counter)

    for r in gap_rows:
        c = classify_row_policy(r)
        bucket = c["bucket"]
        label = c["label"]
        module_id = c["module"] or str(r.get("module_id") or "")
        bucket_counts[bucket] += 1
        label_counts[label] += 1
        if bucket == "actionable" and module_id:
            actionable_by_platform[str(r.get("platform") or "")][module_id] += 1

    actionable_top: Dict[str, List[Dict[str, Any]]] = {}
    for platform in sorted(actionable_by_platform.keys()):
        actionable_top[platform] = [
            {"module_id": mid, "count": int(cnt)}
            for mid, cnt in actionable_by_platform[platform].most_common(8)
        ]

    return {
        "gap_row_count": len(gap_rows),
        "bucket_counts": dict(bucket_counts),
        "label_counts": dict(label_counts),
        "actionable_top_modules_by_platform": actionable_top,
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


def write_json(rows: List[Dict[str, str]], summary: Dict[str, Any], gap_report: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "version": "1.0.0",
        "generator": "tools/generate_driver_capability_matrix.py",
        "row_count": len(rows),
        "summary": summary,
        "gap_report": gap_report,
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


def write_gap_report_json(gap_report: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(gap_report, indent=2) + "\n", encoding="utf-8")


def write_gap_report_md(gap_report: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    lines: List[str] = []
    lines.append("# Driver Capability Gap Report v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_driver_capability_matrix.py`.")
    lines.append("")
    lines.append(f"- Platforms with gap rows: {int(gap_report.get('platform_count', 0))}")
    lines.append("")

    policy = gap_report.get("policy_summary") or {}
    lines.append("## Policy View")
    lines.append("")
    lines.append(f"- Mismatch rows considered: {int(policy.get('mismatch_row_count', 0))}")
    b = policy.get("bucket_counts") or {}
    lines.append(f"- Intentional mismatch rows: {int(b.get('intentional', 0))}")
    lines.append(f"- Actionable mismatch rows: {int(b.get('actionable', 0))}")
    lines.append("")
    lines.append("| Policy Label | Count |")
    lines.append("|---|---|")
    for label, count in sorted((policy.get("label_counts") or {}).items(), key=lambda kv: (-int(kv[1]), kv[0])):
        lines.append(f"| {label} | {int(count)} |")
    lines.append("")

    actionable = policy.get("actionable_top_modules_by_platform") or {}
    if actionable:
        lines.append("### Actionable Top Modules By Platform")
        lines.append("")

    advanced_policy = gap_report.get("advanced_policy_summary") or {}
    lines.append("## Advanced Peripheral Policy View")
    lines.append("")
    lines.append(f"- `not_allowed:*` rows considered: {int(advanced_policy.get('not_allowed_row_count', 0))}")
    ab = advanced_policy.get("bucket_counts") or {}
    lines.append(f"- Intentional `not_allowed` rows: {int(ab.get('intentional', 0))}")
    lines.append(f"- Actionable `not_allowed` rows: {int(ab.get('actionable', 0))}")
    lines.append("")
    lines.append("| Advanced Policy Label | Count |")
    lines.append("|---|---|")
    for label, count in sorted((advanced_policy.get("label_counts") or {}).items(), key=lambda kv: (-int(kv[1]), kv[0])):
        lines.append(f"| {label} | {int(count)} |")
    lines.append("")

    actionable_adv = advanced_policy.get("actionable_top_modules_by_platform") or {}
    if actionable_adv:
        lines.append("### Advanced Policy Actionable Top Modules By Platform")
        lines.append("")
        for platform in sorted(actionable_adv.keys()):
            lines.append(f"- {platform}: " + ", ".join(
                f"{item.get('module_id')} ({int(item.get('count', 0))})" for item in actionable_adv.get(platform, [])
            ))
        lines.append("")

    focus = gap_report.get("foundation_focus_summary") or {}
    lines.append("## Foundation Focus View")
    lines.append("")
    lines.append(f"- Total gap rows considered: {int(focus.get('gap_row_count', 0))}")
    fb = focus.get("bucket_counts") or {}
    lines.append(f"- Actionable rows: {int(fb.get('actionable', 0))}")
    lines.append(f"- Intentional rows: {int(fb.get('intentional', 0))}")
    lines.append("")
    lines.append("| Foundation Focus Label | Count |")
    lines.append("|---|---|")
    for label, count in sorted((focus.get("label_counts") or {}).items(), key=lambda kv: (-int(kv[1]), kv[0])):
        lines.append(f"| {label} | {int(count)} |")
    lines.append("")

    actionable_focus = focus.get("actionable_top_modules_by_platform") or {}
    if actionable_focus:
        lines.append("### Foundation Actionable Top Modules By Platform")
        lines.append("")
        for platform in sorted(actionable_focus.keys()):
            lines.append(f"- {platform}: " + ", ".join(
                f"{item.get('module_id')} ({int(item.get('count', 0))})" for item in actionable_focus.get(platform, [])
            ))
        lines.append("")
        for platform in sorted(actionable.keys()):
            lines.append(f"- {platform}: " + ", ".join(
                f"{item.get('module_id')} ({int(item.get('count', 0))})" for item in actionable.get(platform, [])
            ))
        lines.append("")

    platforms = gap_report.get("platforms") or {}
    for platform in sorted(platforms.keys()):
        pdata = platforms[platform] or {}
        lines.append(f"## {platform}")
        lines.append("")
        lines.append(f"- Gap rows: {int(pdata.get('unsupported_or_gap_rows', 0))}")
        lines.append("")
        lines.append("| Top Module Gaps | Count |")
        lines.append("|---|---|")
        for item in (pdata.get("top_module_gaps") or [])[:8]:
            lines.append(f"| {item.get('module_id', '')} | {int(item.get('count', 0))} |")
        lines.append("")
        lines.append("| Top Reasons | Count |")
        lines.append("|---|---|")
        for item in (pdata.get("top_reasons") or [])[:6]:
            lines.append(f"| {item.get('reason', '')} | {int(item.get('count', 0))} |")
        lines.append("")
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate board x driver capability matrix artifacts.")
    ap.add_argument("--csv-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.csv", help="CSV output path")
    ap.add_argument("--json-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.json", help="JSON output path")
    ap.add_argument("--md-out", default="docs/planning/DRIVER_CAPABILITY_MATRIX.md", help="Markdown output path")
    ap.add_argument("--gap-json-out", default="docs/planning/DRIVER_CAPABILITY_GAPS.json", help="Gap report JSON output path")
    ap.add_argument("--gap-md-out", default="docs/planning/DRIVER_CAPABILITY_GAPS.md", help="Gap report markdown output path")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    csv_out = (root / args.csv_out).resolve()
    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()
    gap_json_out = (root / args.gap_json_out).resolve()
    gap_md_out = (root / args.gap_md_out).resolve()

    modules = discover_modules(root)
    boards = discover_boards(root)
    rows = build_rows(modules, boards)
    summary = build_summary(rows)
    gap_report = build_gap_report(rows)
    gap_report["policy_summary"] = build_policy_summary(rows)
    gap_report["advanced_policy_summary"] = build_advanced_policy_summary(rows)
    gap_report["foundation_focus_summary"] = build_foundation_focus_summary(rows)

    write_csv(rows, csv_out)
    write_json(rows, summary, gap_report, json_out)
    write_md(summary, md_out)
    write_gap_report_json(gap_report, gap_json_out)
    write_gap_report_md(gap_report, gap_md_out)

    print(f"[driver-matrix] boards: {len(boards)}")
    print(f"[driver-matrix] modules: {len(modules)}")
    print(f"[driver-matrix] rows: {len(rows)}")
    print(f"[driver-matrix] wrote: {csv_out}")
    print(f"[driver-matrix] wrote: {json_out}")
    print(f"[driver-matrix] wrote: {md_out}")
    print(f"[driver-matrix] wrote: {gap_json_out}")
    print(f"[driver-matrix] wrote: {gap_md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
