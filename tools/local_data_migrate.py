#!/usr/bin/env python3
"""
Migrate common local-only BasaltOS directories into a canonical local data root.

Default behavior is dry-run. Use --apply to perform moves.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import List


@dataclass
class PlannedMove:
    source: Path
    dest: Path
    category: str


def utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def resolve_dest_root(repo_root: Path, arg_dest: str | None) -> Path:
    raw = arg_dest or os.environ.get("BASALTOS_LOCAL_DATA_DIR", ".basaltos-local")
    p = Path(raw)
    if not p.is_absolute():
        p = repo_root / p
    return p.resolve()


def plan_moves(repo_root: Path, dest_root: Path) -> List[PlannedMove]:
    moves: List[PlannedMove] = []

    # Build output folders
    build_candidates = [repo_root / "build"]
    build_candidates.extend(sorted(repo_root.glob("build_*")))
    for src in build_candidates:
        if not src.exists() or not src.is_dir():
            continue
        moves.append(
            PlannedMove(
                source=src,
                dest=dest_root / "cache" / "builds" / src.name,
                category="build_cache",
            )
        )

    # Known ad-hoc local data folders seen across experiments.
    legacy_names = [".basaltos", "basalt_local_data", "local_data"]
    for name in legacy_names:
        src = repo_root / name
        if not src.exists():
            continue
        moves.append(
            PlannedMove(
                source=src,
                dest=dest_root / "legacy" / name,
                category="legacy_local",
            )
        )

    # Do not move destination root into itself.
    clean: List[PlannedMove] = []
    for m in moves:
        try:
            m.source.resolve().relative_to(dest_root.resolve())
            continue
        except ValueError:
            clean.append(m)
    return clean


def uniquify_dest(path: Path) -> Path:
    if not path.exists():
        return path
    suffix = utc_stamp()
    return path.with_name(f"{path.name}_{suffix}")


def apply_moves(moves: List[PlannedMove], apply: bool) -> List[dict]:
    results = []
    for m in moves:
        row = {
            "source": str(m.source),
            "dest": str(m.dest),
            "category": m.category,
            "status": "planned",
            "note": "",
        }
        if not apply:
            results.append(row)
            continue

        if not m.source.exists():
            row["status"] = "skipped"
            row["note"] = "source no longer exists"
            results.append(row)
            continue

        final_dest = uniquify_dest(m.dest)
        final_dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(m.source), str(final_dest))
        row["dest"] = str(final_dest)
        row["status"] = "moved"
        results.append(row)

    return results


def write_report(dest_root: Path, results: List[dict]) -> Path:
    report_dir = dest_root / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    out = report_dir / f"migration_{utc_stamp()}.json"
    payload = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "items": results,
    }
    out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".", help="Repository root")
    ap.add_argument(
        "--dest-root",
        default=None,
        help="Destination local data root (default: BASALTOS_LOCAL_DATA_DIR or .basaltos-local)",
    )
    ap.add_argument("--apply", action="store_true", help="Perform moves (default is dry-run)")
    ap.add_argument("--dry-run", action="store_true", help="Force dry-run")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    dest_root = resolve_dest_root(repo_root, args.dest_root)
    do_apply = bool(args.apply and not args.dry_run)

    moves = plan_moves(repo_root, dest_root)
    if not moves:
        print("[local-data] no migration candidates found")
        return 0

    print(f"[local-data] repo root: {repo_root}")
    print(f"[local-data] destination root: {dest_root}")
    print(f"[local-data] mode: {'apply' if do_apply else 'dry-run'}")
    print("[local-data] planned moves:")
    for m in moves:
        print(f"  - {m.source} -> {m.dest} ({m.category})")

    results = apply_moves(moves, do_apply)
    if do_apply:
        report = write_report(dest_root, results)
        print(f"[local-data] report: {report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
