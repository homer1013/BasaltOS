#!/usr/bin/env python3
"""
Export local BasaltOS guest data to a versioned sync envelope (schema 1.0.0).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import socket
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

SCHEMA_VERSION = "1.0.0"


def now_utc_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def ts_compact() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def file_utc_iso(path: Path) -> str:
    dt = datetime.fromtimestamp(path.stat().st_mtime, tz=timezone.utc).replace(microsecond=0)
    return dt.isoformat().replace("+00:00", "Z")


def normalize_slug(s: str) -> str:
    out = re.sub(r"[^a-zA-Z0-9._-]+", "_", s.strip())
    out = out.strip("_")
    return out or "item"


def canonical_json_hash(payload: Any) -> str:
    blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return f"sha256:{hashlib.sha256(blob).hexdigest()}"


def parse_file_content(path: Path) -> Any:
    text = path.read_text(encoding="utf-8")
    if path.suffix.lower() == ".json":
        return json.loads(text)
    return text


def deterministic_device_id() -> str:
    host = socket.gethostname()
    node = uuid.getnode()
    raw = f"{host}:{node}".encode("utf-8")
    return "dev_" + hashlib.sha256(raw).hexdigest()[:16]


def add_item(items: list[dict[str, Any]], item_type: str, item_id: str, updated_utc: str, content: Any) -> None:
    items.append(
        {
            "item_type": item_type,
            "item_id": item_id,
            "updated_utc": updated_utc,
            "content_hash": canonical_json_hash(content),
            "content": content,
        }
    )


def collect_generated_snapshot(repo_root: Path, items: list[dict[str, Any]]) -> None:
    generated = repo_root / "config" / "generated"
    if not generated.exists():
        return
    tracked = [
        "basalt_config.json",
        "basalt.features.json",
        "applets.json",
        "basalt_config.h",
        "sdkconfig.defaults",
    ]
    content: dict[str, Any] = {}
    mtimes: list[float] = []
    for name in tracked:
        p = generated / name
        if not p.is_file():
            continue
        mtimes.append(p.stat().st_mtime)
        content[name] = parse_file_content(p)

    if not content:
        return
    latest = datetime.fromtimestamp(max(mtimes), tz=timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    add_item(
        items,
        item_type="config_snapshot",
        item_id="config_generated_current",
        updated_utc=latest,
        content={"scope": "config/generated", "files": content},
    )


def collect_local_folder_items(base: Path, item_type: str, id_prefix: str, items: list[dict[str, Any]]) -> None:
    if not base.exists():
        return
    for p in sorted(base.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(base)
        item_id = f"{id_prefix}_{normalize_slug(str(rel))}"
        add_item(
            items,
            item_type=item_type,
            item_id=item_id,
            updated_utc=file_utc_iso(p),
            content={"path": str(rel), "data": parse_file_content(p)},
        )


def build_envelope(repo_root: Path, local_root: Path, app_version: str) -> dict[str, Any]:
    items: list[dict[str, Any]] = []
    collect_generated_snapshot(repo_root, items)
    collect_local_folder_items(local_root / "user" / "configs", "config_snapshot", "config_local", items)
    collect_local_folder_items(local_root / "user" / "boards", "board_inventory", "board_local", items)
    collect_local_folder_items(local_root / "user" / "presets", "user_preset", "preset_local", items)
    return {
        "schema_version": SCHEMA_VERSION,
        "source": {
            "kind": "basaltos_main_local",
            "app_version": str(app_version),
            "device_id": deterministic_device_id(),
        },
        "exported_utc": now_utc_iso(),
        "items": items,
    }


def resolve_local_data_root(repo_root: Path, arg_path: str | None) -> Path:
    raw = arg_path or os.environ.get("BASALTOS_LOCAL_DATA_DIR", ".basaltos-local")
    p = Path(raw)
    if not p.is_absolute():
        p = repo_root / p
    return p.resolve()


def main() -> int:
    ap = argparse.ArgumentParser(description="Export local BasaltOS sync payload envelope")
    ap.add_argument("--repo-root", default=".", help="Repository root")
    ap.add_argument("--local-data-root", default=None, help="Local data root (default: BASALTOS_LOCAL_DATA_DIR or .basaltos-local)")
    ap.add_argument("--app-version", default="v0.1.0", help="App version marker in source block")
    ap.add_argument("--out", default=None, help="Output JSON path")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    local_root = resolve_local_data_root(repo_root, args.local_data_root)

    out_path = Path(args.out).resolve() if args.out else (local_root / "user" / "exports" / f"sync_export_{ts_compact()}.json")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    envelope = build_envelope(repo_root, local_root, args.app_version)
    out_path.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")

    counts: dict[str, int] = {}
    for i in envelope["items"]:
        t = str(i.get("item_type"))
        counts[t] = counts.get(t, 0) + 1
    print(f"[sync-export] wrote: {out_path}")
    print(f"[sync-export] items: {len(envelope['items'])}")
    for k in sorted(counts):
        print(f"[sync-export]   {k}: {counts[k]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
