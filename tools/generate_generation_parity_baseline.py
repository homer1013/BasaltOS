#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path
from typing import Dict, List


REPRESENTATIVE_BOARDS: Dict[str, str] = {
    "atmega": "arduino_uno_r3",
    "atsam": "adafruit_circuit_playground_express",
    "avr": "mega2560",
    "esp32": "esp32-c3-supermini",
    "linux-sbc": "raspberry_pi_4b",
    "pic16": "curiosity_nano_pic16f13145",
    "renesas_ra": "arduino_uno_r4_wifi",
    "renesas_rx": "rx65n_envision_kit",
    "rp2040": "raspberry_pi_pico",
    "stm32": "nucleo_f401re",
}

REQUIRED_ARTIFACTS = [
    "basalt_config.h",
    "basalt.features.json",
    "sdkconfig.defaults",
    "basalt_config.json",
]


def load_board_defaults(root: Path, platform: str, board: str) -> List[str]:
    board_json = root / "boards" / platform / board / "board.json"
    if not board_json.exists():
        return []
    try:
        board_data = json.loads(board_json.read_text(encoding="utf-8"))
    except Exception:
        return []
    return sorted(
        str(x).strip()
        for x in (((board_data.get("defaults") or {}).get("modules") or []))
        if str(x).strip()
    )


def run_case(
    root: Path,
    platform: str,
    board: str,
    out_root: Path,
    mode: str,
    enabled_drivers: List[str],
    board_defaults: List[str],
) -> Dict[str, object]:
    outdir = out_root / mode / platform / board
    outdir.mkdir(parents=True, exist_ok=True)
    log_path = outdir / "run.log"
    outdir_arg = str(outdir.relative_to(root))

    cmd = [
        "python3",
        "tools/configure.py",
        "--platform",
        platform,
        "--board",
        board,
        "--outdir",
        outdir_arg,
    ]
    if enabled_drivers:
        cmd.extend(["--enable-drivers", ",".join(enabled_drivers)])

    p = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    log_path.write_text((p.stdout or "") + ("\n" if p.stdout else "") + (p.stderr or ""), encoding="utf-8")

    artifacts = {name: (outdir / name).exists() for name in REQUIRED_ARTIFACTS}
    missing = [k for k, ok in artifacts.items() if not ok]

    enabled_modules: List[str] = []
    features_json = outdir / "basalt.features.json"
    if features_json.exists():
        try:
            features_data = json.loads(features_json.read_text(encoding="utf-8"))
            enabled_modules = sorted(str(x).strip() for x in (features_data.get("modules") or []) if str(x).strip())
        except Exception:
            enabled_modules = []

    missing_defaults = sorted(set(board_defaults) - set(enabled_modules))
    parity = {
        "enabled_module_count": len(enabled_modules),
        "default_module_count": len(board_defaults),
        "missing_default_module_count": len(missing_defaults),
        "missing_default_modules": missing_defaults,
    }

    return {
        "mode": mode,
        "platform": platform,
        "board_dir": board,
        "requested_enabled_drivers": list(enabled_drivers),
        "command": " ".join(cmd),
        "exit_code": int(p.returncode),
        "artifacts": artifacts,
        "missing_artifacts": missing,
        "status": "PASS" if p.returncode == 0 and not missing else "FAIL",
        "log_path": str(log_path.relative_to(root)),
        "module_parity": parity,
    }


def build_row(root: Path, platform: str, board: str, out_root: Path) -> Dict[str, object]:
    board_defaults = load_board_defaults(root, platform, board)
    empty_run = run_case(
        root=root,
        platform=platform,
        board=board,
        out_root=out_root,
        mode="empty",
        enabled_drivers=[],
        board_defaults=board_defaults,
    )
    defaults_run = run_case(
        root=root,
        platform=platform,
        board=board,
        out_root=out_root,
        mode="with_defaults",
        enabled_drivers=board_defaults,
        board_defaults=board_defaults,
    )

    empty_parity = empty_run.get("module_parity") or {}
    defaults_parity = defaults_run.get("module_parity") or {}
    comparison = {
        "enabled_module_delta": int(defaults_parity.get("enabled_module_count", 0)) - int(
            empty_parity.get("enabled_module_count", 0)
        ),
        "missing_default_module_delta": int(defaults_parity.get("missing_default_module_count", 0)) - int(
            empty_parity.get("missing_default_module_count", 0)
        ),
        "all_defaults_resolved_in_defaults_mode": int(defaults_parity.get("missing_default_module_count", 0)) == 0,
    }

    return {
        "platform": platform,
        "board_dir": board,
        "status": "PASS" if empty_run["status"] == "PASS" and defaults_run["status"] == "PASS" else "FAIL",
        "board_defaults": board_defaults,
        "runs": {
            "empty": empty_run,
            "with_defaults": defaults_run,
        },
        "comparison": comparison,
    }


def write_json(path: Path, payload: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def build_gap_summary(rows: List[Dict[str, object]]) -> Dict[str, object]:
    max_enabled_defaults_mode = max(
        (
            int((((r.get("runs") or {}).get("with_defaults") or {}).get("module_parity") or {}).get("enabled_module_count", 0))
            for r in rows
        ),
        default=0,
    )
    with_unresolved_defaults = [
        r
        for r in rows
        if int((((r.get("runs") or {}).get("with_defaults") or {}).get("module_parity") or {}).get("missing_default_module_count", 0)) > 0
    ]
    by_platform: Dict[str, Dict[str, int]] = {}
    for r in rows:
        empty_parity = (((r.get("runs") or {}).get("empty") or {}).get("module_parity") or {})
        defaults_parity = (((r.get("runs") or {}).get("with_defaults") or {}).get("module_parity") or {})
        by_platform[str(r.get("platform"))] = {
            "empty_enabled_module_count": int(empty_parity.get("enabled_module_count", 0)),
            "defaults_enabled_module_count": int(defaults_parity.get("enabled_module_count", 0)),
            "defaults_missing_default_module_count": int(defaults_parity.get("missing_default_module_count", 0)),
        }
    return {
        "max_enabled_module_count_in_defaults_mode": max_enabled_defaults_mode,
        "platforms_with_unresolved_defaults_in_defaults_mode": len(with_unresolved_defaults),
        "platforms_with_fully_resolved_defaults": len(rows) - len(with_unresolved_defaults),
        "by_platform": by_platform,
    }


def write_md(path: Path, rows: List[Dict[str, object]], out_root: Path, gap_summary: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    pass_count = sum(1 for r in rows if r["status"] == "PASS")
    fail_count = len(rows) - pass_count
    empty_mode_fail_count = sum(
        1 for r in rows if (((r.get("runs") or {}).get("empty") or {}).get("status") != "PASS")
    )
    with_defaults_mode_fail_count = sum(
        1 for r in rows if (((r.get("runs") or {}).get("with_defaults") or {}).get("status") != "PASS")
    )
    lines: List[str] = []
    lines.append("# Cross-Platform Generation Parity Baseline")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_generation_parity_baseline.py`.")
    lines.append("")
    lines.append(f"- Platforms covered: {len(rows)}")
    lines.append(f"- PASS: {pass_count}")
    lines.append(f"- FAIL: {fail_count}")
    lines.append(f"- Empty mode FAIL (gating): {empty_mode_fail_count}")
    lines.append(f"- With defaults FAIL (non-gating parity signal): {with_defaults_mode_fail_count}")
    lines.append(f"- Artifact root: `{out_root}`")
    lines.append(
        f"- Max enabled modules in `with_defaults` mode: {gap_summary['max_enabled_module_count_in_defaults_mode']}"
    )
    lines.append(
        "- Platforms with unresolved defaults in `with_defaults` mode: "
        f"{gap_summary['platforms_with_unresolved_defaults_in_defaults_mode']}"
    )
    lines.append("")
    lines.append("| Platform | Representative Board | Status | Empty Log | With Defaults Log |")
    lines.append("|---|---|---|---|---|")
    for r in rows:
        empty_log = ((r.get("runs") or {}).get("empty") or {}).get("log_path", "-")
        defaults_log = ((r.get("runs") or {}).get("with_defaults") or {}).get("log_path", "-")
        lines.append(
            f"| {r['platform']} | {r['board_dir']} | {r['status']} | `{empty_log}` | `{defaults_log}` |"
        )
    lines.append("")
    lines.append("## Driver/Module Parity Comparison")
    lines.append("")
    lines.append(
        "| Platform | Empty Enabled | Defaults Enabled | Board Default Modules | Missing Defaults (Empty) | Missing Defaults (With Defaults) |"
    )
    lines.append("|---|---|---|---|---|---|")
    for r in rows:
        empty_parity = (((r.get("runs") or {}).get("empty") or {}).get("module_parity") or {})
        defaults_parity = (((r.get("runs") or {}).get("with_defaults") or {}).get("module_parity") or {})
        lines.append(
            "| "
            f"{r['platform']} | "
            f"{empty_parity.get('enabled_module_count', 0)} | "
            f"{defaults_parity.get('enabled_module_count', 0)} | "
            f"{defaults_parity.get('default_module_count', 0)} | "
            f"{empty_parity.get('missing_default_module_count', 0)} | "
            f"{defaults_parity.get('missing_default_module_count', 0)} |"
        )
    lines.append("")
    lines.append("## Gap Summary")
    lines.append("")
    for r in rows:
        missing = ((((r.get("runs") or {}).get("with_defaults") or {}).get("module_parity") or {}).get("missing_default_modules") or [])
        if missing:
            lines.append(f"- {r['platform']}: unresolved defaults in with_defaults mode -> {', '.join(missing)}")
        else:
            lines.append(f"- {r['platform']}: all board defaults resolved in with_defaults mode")
    lines.append("")
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate cross-platform configure parity baseline artifacts.")
    ap.add_argument("--out-json", default="docs/planning/GENERATION_PARITY_BASELINE.json", help="JSON output path")
    ap.add_argument("--out-md", default="docs/planning/GENERATION_PARITY_BASELINE.md", help="Markdown output path")
    ap.add_argument(
        "--artifact-root",
        default="tmp/generation_parity_baseline",
        help="Output root for per-platform configure artifacts/logs",
    )
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    out_json = (root / args.out_json).resolve()
    out_md = (root / args.out_md).resolve()
    artifact_root = (root / args.artifact_root).resolve()

    rows: List[Dict[str, object]] = []
    for platform in sorted(REPRESENTATIVE_BOARDS.keys()):
        rows.append(build_row(root, platform, REPRESENTATIVE_BOARDS[platform], artifact_root))

    gap_summary = build_gap_summary(rows)

    payload = {
        "version": "1.2.0",
        "run_modes": ["empty", "with_defaults"],
        "representative_boards": REPRESENTATIVE_BOARDS,
        "required_artifacts": REQUIRED_ARTIFACTS,
        "rows": rows,
        "module_parity_gap_summary": gap_summary,
        "summary": {
            "platform_count": len(rows),
            "pass_count": sum(1 for r in rows if r["status"] == "PASS"),
            "fail_count": sum(1 for r in rows if r["status"] != "PASS"),
            "empty_mode_fail_count": sum(
                1 for r in rows if (((r.get("runs") or {}).get("empty") or {}).get("status") != "PASS")
            ),
            "with_defaults_mode_fail_count": sum(
                1 for r in rows if (((r.get("runs") or {}).get("with_defaults") or {}).get("status") != "PASS")
            ),
        },
    }
    write_json(out_json, payload)
    write_md(out_md, rows, artifact_root.relative_to(root), gap_summary)

    print(f"[parity] wrote: {out_json}")
    print(f"[parity] wrote: {out_md}")
    print(f"[parity] artifact root: {artifact_root}")
    if payload["summary"]["empty_mode_fail_count"] > 0:
        print(f"[parity] empty-mode failures: {payload['summary']['empty_mode_fail_count']}")
        print(f"[parity] with-defaults failures: {payload['summary']['with_defaults_mode_fail_count']}")
        return 1
    if payload["summary"]["with_defaults_mode_fail_count"] > 0:
        print(f"[parity] with-defaults failures (non-gating): {payload['summary']['with_defaults_mode_fail_count']}")
    print("[parity] all representative platforms passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
