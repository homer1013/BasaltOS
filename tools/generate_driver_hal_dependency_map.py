#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Set


def _read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _discover_modules(root: Path) -> List[Dict[str, Any]]:
    rows: List[Dict[str, Any]] = []
    for module_json in sorted((root / "modules").glob("**/module.json")):
        payload = _read_json(module_json)
        depends = [str(x).strip() for x in (payload.get("depends") or []) if str(x).strip()]
        depends = sorted(dict.fromkeys(depends))
        rows.append(
            {
                "id": str(payload.get("id") or module_json.parent.name),
                "name": str(payload.get("name") or payload.get("id") or module_json.parent.name),
                "path": str(module_json.relative_to(root)),
                "depends": depends,
            }
        )
    return rows


def _build_report(root: Path) -> Dict[str, Any]:
    modules = _discover_modules(root)
    hal_matrix = _read_json(root / "docs/planning/HAL_ADAPTER_MATRIX.json")
    completeness = _read_json(root / "docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json")

    hal_primitives: Set[str] = set(hal_matrix.get("primitives") or [])
    required_rows = completeness.get("rows") or []
    required_ports = [str(r.get("port") or "") for r in required_rows]
    missing_by_port = {str(r.get("port") or ""): set(r.get("missing_primitives") or []) for r in required_rows}

    unique_deps: Set[str] = set()
    hal_deps: Set[str] = set()
    non_hal_deps: Set[str] = set()
    modules_with_dep = 0
    modules_with_unmapped = 0
    modules_with_coverage_gap = 0

    module_rows: List[Dict[str, Any]] = []
    dependency_rows: List[Dict[str, Any]] = []

    for mod in sorted(modules, key=lambda r: r["id"]):
        deps = mod["depends"]
        if deps:
            modules_with_dep += 1
        analyses: List[Dict[str, Any]] = []
        has_unmapped = False
        has_gap = False
        for dep in deps:
            unique_deps.add(dep)
            if dep in hal_primitives:
                hal_deps.add(dep)
                blocking_ports = sorted([p for p in required_ports if dep in missing_by_port.get(p, set())])
                risk = "coverage_gap" if blocking_ports else "none"
                if blocking_ports:
                    has_gap = True
                analyses.append(
                    {
                        "dependency": dep,
                        "dependency_type": "hal_primitive",
                        "risk": risk,
                        "blocking_ports": blocking_ports,
                    }
                )
            else:
                non_hal_deps.add(dep)
                has_unmapped = True
                analyses.append(
                    {
                        "dependency": dep,
                        "dependency_type": "non_hal_dependency",
                        "risk": "unmapped_dependency",
                        "blocking_ports": [],
                    }
                )

        if has_unmapped:
            modules_with_unmapped += 1
        if has_gap:
            modules_with_coverage_gap += 1

        module_rows.append(
            {
                "id": mod["id"],
                "name": mod["name"],
                "path": mod["path"],
                "depends": deps,
                "analysis": analyses,
                "has_unmapped_dependency": has_unmapped,
                "has_coverage_gap": has_gap,
            }
        )

        for a in analyses:
            dependency_rows.append(
                {
                    "module_id": mod["id"],
                    "dependency": a["dependency"],
                    "dependency_type": a["dependency_type"],
                    "risk": a["risk"],
                    "blocking_ports": a["blocking_ports"],
                }
            )

    summary = {
        "modules_total": len(modules),
        "modules_with_dependencies": modules_with_dep,
        "unique_dependencies": len(unique_deps),
        "hal_dependencies": len(hal_deps),
        "non_hal_dependencies": len(non_hal_deps),
        "modules_with_unmapped_dependency": modules_with_unmapped,
        "modules_with_coverage_gap": modules_with_coverage_gap,
        "required_adapter_count": len(required_ports),
    }

    return {
        "version": "1.0.0",
        "generator": "tools/generate_driver_hal_dependency_map.py",
        "sources": {
            "modules_glob": "modules/**/module.json",
            "hal_matrix": "docs/planning/HAL_ADAPTER_MATRIX.json",
            "adapter_completeness": "docs/planning/HAL_PLATFORM_ADAPTER_COMPLETENESS.json",
        },
        "summary": summary,
        "dependencies": sorted(dependency_rows, key=lambda r: (r["dependency"], r["module_id"])),
        "modules": module_rows,
    }


def _write_md(payload: Dict[str, Any], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    summary = payload.get("summary") or {}
    modules = payload.get("modules") or []

    lines: List[str] = []
    lines.append("# Driver-to-HAL Dependency Map v1")
    lines.append("")
    lines.append("Auto-generated by `tools/generate_driver_hal_dependency_map.py`.")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Modules total: {summary.get('modules_total', 0)}")
    lines.append(f"- Modules with dependencies: {summary.get('modules_with_dependencies', 0)}")
    lines.append(f"- Unique dependencies: {summary.get('unique_dependencies', 0)}")
    lines.append(f"- HAL dependencies: {summary.get('hal_dependencies', 0)}")
    lines.append(f"- Non-HAL dependencies: {summary.get('non_hal_dependencies', 0)}")
    lines.append(f"- Modules with unmapped dependency: {summary.get('modules_with_unmapped_dependency', 0)}")
    lines.append(f"- Modules with adapter coverage gap: {summary.get('modules_with_coverage_gap', 0)}")
    lines.append("")
    lines.append("| Module | Dependencies | Risk Flags |")
    lines.append("|---|---|---|")
    for mod in modules:
        deps = ", ".join(mod.get("depends") or []) or "-"
        flags: List[str] = []
        if mod.get("has_unmapped_dependency"):
            flags.append("unmapped-dependency")
        if mod.get("has_coverage_gap"):
            flags.append("coverage-gap")
        risk = ", ".join(flags) if flags else "none"
        lines.append(f"| {mod.get('id')} | {deps} | {risk} |")
    lines.append("")
    out_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate driver-to-HAL dependency map artifacts.")
    ap.add_argument("--json-out", default="docs/planning/DRIVER_HAL_DEPENDENCY_MAP.json")
    ap.add_argument("--md-out", default="docs/planning/DRIVER_HAL_DEPENDENCY_MAP.md")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    payload = _build_report(root)

    json_out = (root / args.json_out).resolve()
    md_out = (root / args.md_out).resolve()
    json_out.parent.mkdir(parents=True, exist_ok=True)

    json_out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    _write_md(payload, md_out)

    summary = payload["summary"]
    print(f"[driver-hal] modules: {summary['modules_total']}")
    print(f"[driver-hal] unique dependencies: {summary['unique_dependencies']}")
    print(f"[driver-hal] unmapped modules: {summary['modules_with_unmapped_dependency']}")
    print(f"[driver-hal] coverage-gap modules: {summary['modules_with_coverage_gap']}")
    print(f"[driver-hal] wrote: {json_out}")
    print(f"[driver-hal] wrote: {md_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
