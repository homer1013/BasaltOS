#!/usr/bin/env python3
"""
Import a sync payload envelope into local BasaltOS workspace folders.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SUPPORTED_SCHEMA = "1.0.0"


def utc_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def ts_compact() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def canonical_hash(payload: Any) -> str:
    blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return "sha256:" + hashlib.sha256(blob).hexdigest()


def resolve_local_data_root(repo_root: Path, arg_path: str | None) -> Path:
    raw = arg_path or os.environ.get("BASALTOS_LOCAL_DATA_DIR", ".basaltos-local")
    p = Path(raw)
    if not p.is_absolute():
        p = repo_root / p
    return p.resolve()


def path_for_item(root: Path, item_type: str, item_id: str) -> Path | None:
    safe_id = "".join(ch if ch.isalnum() or ch in ("_", "-", ".") else "_" for ch in item_id).strip("_.")
    if not safe_id:
        safe_id = "item"
    if item_type == "config_snapshot":
        return root / "user" / "configs" / f"{safe_id}.json"
    if item_type == "board_inventory":
        return root / "user" / "boards" / f"{safe_id}.json"
    if item_type == "user_preset":
        return root / "user" / "presets" / f"{safe_id}.json"
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description="Import BasaltOS sync payload into local workspace")
    ap.add_argument("--payload", required=True, help="Path to payload JSON")
    ap.add_argument("--repo-root", default=".", help="Repository root")
    ap.add_argument("--local-data-root", default=None, help="Local data root")
    ap.add_argument(
        "--policy",
        choices=["local_preferred", "cloud_preferred", "manual_per_item"],
        default="manual_per_item",
        help="Conflict policy",
    )
    ap.add_argument(
        "--allow-item",
        action="append",
        default=[],
        help="For manual_per_item, allow import of specific item_id (repeatable)",
    )
    ap.add_argument("--apply", action="store_true", help="Write imported files")
    ap.add_argument("--dry-run", action="store_true", help="Force dry-run mode")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    local_root = resolve_local_data_root(repo_root, args.local_data_root)
    payload_path = Path(args.payload).resolve()
    data = json.loads(payload_path.read_text(encoding="utf-8"))

    schema = str(data.get("schema_version", ""))
    if schema != SUPPORTED_SCHEMA:
        raise SystemExit(f"unsupported schema_version '{schema}', expected '{SUPPORTED_SCHEMA}'")

    do_apply = bool(args.apply and not args.dry_run)
    allow = set(str(x) for x in (args.allow_item or []))
    results: list[dict[str, Any]] = []

    for item in data.get("items", []) or []:
        if not isinstance(item, dict):
            continue
        item_type = str(item.get("item_type", "")).strip()
        item_id = str(item.get("item_id", "")).strip()
        content = item.get("content")
        if not item_type or not item_id:
            continue

        dst = path_for_item(local_root, item_type, item_id)
        if dst is None:
            results.append({"item_id": item_id, "item_type": item_type, "status": "skipped", "reason": "unsupported_item_type"})
            continue

        incoming_hash = canonical_hash(content)
        existing_hash = None
        if dst.exists():
            existing_hash = canonical_hash(json.loads(dst.read_text(encoding="utf-8")))

        status = "planned_create"
        reason = ""
        if existing_hash is not None:
            if existing_hash == incoming_hash:
                status = "skipped"
                reason = "unchanged"
            else:
                if args.policy == "local_preferred":
                    status = "skipped"
                    reason = "conflict_local_preferred"
                elif args.policy == "cloud_preferred":
                    status = "planned_update"
                    reason = "conflict_cloud_preferred"
                else:
                    if item_id in allow:
                        status = "planned_update"
                        reason = "conflict_manual_allowed"
                    else:
                        status = "conflicted"
                        reason = "manual_confirmation_required"

        row = {
            "item_type": item_type,
            "item_id": item_id,
            "dest": str(dst),
            "status": status,
            "reason": reason,
        }
        results.append(row)

        if not do_apply:
            continue
        if status not in {"planned_create", "planned_update"}:
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(json.dumps(content, indent=2) + "\n", encoding="utf-8")
        row["status"] = "created" if status == "planned_create" else "updated"

    counts = {"created": 0, "updated": 0, "conflicted": 0, "skipped": 0, "planned": 0}
    for r in results:
        s = str(r.get("status", ""))
        if s == "created":
            counts["created"] += 1
        elif s == "updated":
            counts["updated"] += 1
        elif s == "conflicted":
            counts["conflicted"] += 1
        elif s == "skipped":
            counts["skipped"] += 1
        elif s.startswith("planned_"):
            counts["planned"] += 1

    report_dir = local_root / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    report_path = report_dir / f"sync_import_{ts_compact()}.json"
    report = {
        "generated_utc": utc_iso(),
        "payload": str(payload_path),
        "policy": args.policy,
        "mode": "apply" if do_apply else "dry-run",
        "counts": counts,
        "items": results,
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    print(f"[sync-import] mode: {'apply' if do_apply else 'dry-run'}")
    print(f"[sync-import] report: {report_path}")
    for k in ("created", "updated", "planned", "conflicted", "skipped"):
        print(f"[sync-import]   {k}: {counts[k]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
