#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Set


def _read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _resolve_required_port(platform: str, mcu: str) -> Optional[str]:
    p = (platform or "").strip().lower()
    m = (mcu or "").strip().upper()

    if p == "esp32":
        if "ESP32-C3" in m:
            return "esp32c3"
        if "ESP32-C6" in m:
            return "esp32c6"
        if "ESP32-H2" in m:
            return "esp32h2"
        if "ESP32-S2" in m:
            return "esp32s2"
        if "ESP32-S3" in m:
            return "esp32s3"
        return "esp32"

    direct = {
        "stm32": "stm32",
        "rp2040": "rp2040",
        "pic16": "pic16",
        "renesas_ra": "ra4m1",
    }
    return direct.get(p)


def _discover_board_requirements(root: Path) -> Dict[str, List[str]]:
    by_port: Dict[str, List[str]] = {}
    for board_json in sorted((root / "boards").glob("**/board.json")):
        payload = _read_json(board_json)
        platform = str(payload.get("platform") or "")
        mcu = str(payload.get("mcu") or "")
        required = _resolve_required_port(platform, mcu)
        if not required:
            continue
        board_ref = str(payload.get("id") or board_json.parent.name)
        by_port.setdefault(required, []).append(board_ref)
    for k in by_port:
        by_port[k] = sorted(set(by_port[k]))
    return dict(sorted(by_port.items(), key=lambda kv: kv[0]))


def _build_report(root: Path) -> Dict[str, Any]:
    matrix = _read_json(root / "docs/planning/HAL_ADAPTER_MATRIX.json")
    policy = _read_json(root / "docs/planning/HAL_CONTRACT_POLICY.json")

    matrix_rows = {str(r.get("port") or ""): r for r in (matrix.get("rows") or [])}
    required_by_port = _discover_board_requirements(root)

    ack_partial: Set[str] = set(policy.get("acknowledged_partial_ports") or [])
    ack_missing: Set[str] = set(policy.get("acknowledged_missing_ports") or [])

    rows: List[Dict[str, Any]] = []
    blocking_gaps = 0
    for port in sorted(required_by_port.keys()):
        matrix_row = matrix_rows.get(port)
        boards = required_by_port[port]
        if matrix_row:
            status = str(matrix_row.get("status") or "")
            present_count = int(matrix_row.get("present_count") or 0)
            missing_count = int(matrix_row.get("missing_count") or 0)
            missing = list(matrix_row.get("missing") or [])
        else:
            status = "missing_adapter"
            present_count = 0
            missing_count = int(matrix.get("summary", {}).get("primitive_count", 0))
            missing = list(matrix.get("primitives") or [])

        if status == "complete":
            contract_state = "pass"
            reason = "Adapter is complete for required platform."
        elif status == "partial":
            if port in ack_partial:
                contract_state = "pass_acknowledged_partial"
                reason = "Partial adapter is explicitly acknowledged in HAL policy."
            else:
                contract_state = "fail_unacknowledged_partial"
                reason = "Partial adapter is not acknowledged in HAL policy."
        elif status in {"missing", "missing_adapter"}:
            if port in ack_missing:
                contract_state = "pass_acknowledged_missing"
                reason = "Missing adapter is explicitly acknowledged in HAL policy."
            else:
                contract_state = "fail_unacknowledged_missing"
                reason = "Missing adapter is not acknowledged in HAL policy."
        else:
            contract_state = "fail_invalid_status"
            reason = f"Unexpected adapter status: {status}"

        if contract_state.startswith("fail_"):
            blocking_gaps += 1

        rows.append(
            {
                "port": port,
                "status": status,
                "contract_state": contract_state,
                "reason": reason,
                "required_board_count": len(boards),
                "required_boards": boards,
                "present_count": present_count,
                "missing_count": missing_count,
                "missing_primitives": missing,
            }
        )

    status_counts: Dict[str, int] = {}
    for row in rows:
        st = row["status"]
        status_counts[st] = status_counts.get(st, 0) + 1

    contract_counts: Dict[str, int] = {}
    for row in rows:
        cs = row["contract_state"]
        contract_counts[cs] = contract_counts.get(cs, 0) + 1

    return {
        "version": "1.0.0",
        "generator": "tools/generate_hal_platform_adapter_completeness.py",
        "sources": {
            "matrix": "docs/planning/HAL_ADAPTER_MATRIX.json",
            "policy": "docs/planning/HAL_CONTRACT_POLICY.json",
            "boards_glob": "boards/**/board.json",
        },
        "summary": {
            "required_adapter_count": len(rows),
            "required_board_count": sum(len(v) for v in required_by_port.values()),
            "status_counts": status_counts,
            "contract_state_counts": contract_counts,
            "blocking_gaps": blocking_gaps,
        },
        "rows": rows,
    }


def _write_md(payload: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    summary = payload.get("summary") or {}
    rows = payload.get("rows") or []
    lines: List[str] = []
    lines.append("# HAL Platform Adapter Completeness v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_hal_platform_adapter_completeness.py`.")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Required adapters: {summary.get('required_adapter_count', 0)}")
    lines.append(f"- Boards mapped to required adapters: {summary.get('required_board_count', 0)}")
    lines.append(f"- Blocking gaps: {summary.get('blocking_gaps', 0)}")
    lines.append("")
    lines.append("| Adapter | Status | Contract State | Required Boards | Missing Primitives |")
    lines.append("|---|---|---|---|---|")
    for row in rows:
        missing = ", ".join(row.get("missing_primitives") or []) or "-"
        lines.append(
            f"| {row.get('port')} | {row.get('status')} | {row.get('contract_state')} "
            f"| {row.get('required_board_count')} | {missing} |"
        )
    lines.append("")
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate HAL platform adapter completeness artifacts.")
    ap.add_argument("--json-out", default="docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json")
    ap.add_argument("--md-out", default="docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.md")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    payload = _build_report(root)

    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()
    json_out.parent.mkdir(parents=True, exist_ok=True)

    json_out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    _write_md(payload, md_out)

    print(f"[hal-platform] required adapters: {payload['summary']['required_adapter_count']}")
    print(f"[hal-platform] blocking gaps: {payload['summary']['blocking_gaps']}")
    print(f"[hal-platform] wrote: {json_out}")
    print(f"[hal-platform] wrote: {md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
