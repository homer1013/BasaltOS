#!/usr/bin/env python3
"""
Diff two sync payload envelopes by item identity and content hash.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def load_payload(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def item_key(item: dict[str, Any]) -> tuple[str, str]:
    return (str(item.get("item_type", "")), str(item.get("item_id", "")))


def item_map(payload: dict[str, Any]) -> dict[tuple[str, str], dict[str, Any]]:
    out: dict[tuple[str, str], dict[str, Any]] = {}
    for item in payload.get("items", []) or []:
        if not isinstance(item, dict):
            continue
        key = item_key(item)
        if key[0] and key[1]:
            out[key] = item
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Diff BasaltOS local sync payload envelopes")
    ap.add_argument("--local", required=True, help="Path to local payload JSON")
    ap.add_argument("--remote", required=True, help="Path to remote/cloud payload JSON")
    ap.add_argument("--json-out", default=None, help="Optional JSON diff output path")
    args = ap.parse_args()

    local = load_payload(Path(args.local))
    remote = load_payload(Path(args.remote))
    lmap = item_map(local)
    rmap = item_map(remote)

    lkeys = set(lmap.keys())
    rkeys = set(rmap.keys())

    local_only = sorted(lkeys - rkeys)
    remote_only = sorted(rkeys - lkeys)
    both = sorted(lkeys & rkeys)

    changed = []
    unchanged = 0
    for k in both:
        lh = str(lmap[k].get("content_hash", ""))
        rh = str(rmap[k].get("content_hash", ""))
        if lh != rh:
            changed.append(k)
        else:
            unchanged += 1

    by_type: dict[str, dict[str, int]] = {}
    for group, name in ((local_only, "local_only"), (remote_only, "remote_only"), (changed, "changed")):
        for t, _iid in group:
            if t not in by_type:
                by_type[t] = {"local_only": 0, "remote_only": 0, "changed": 0}
            by_type[t][name] += 1

    report = {
        "summary": {
            "local_items": len(lkeys),
            "remote_items": len(rkeys),
            "local_only": len(local_only),
            "remote_only": len(remote_only),
            "changed": len(changed),
            "unchanged": unchanged,
        },
        "by_type": by_type,
        "local_only": [{"item_type": t, "item_id": iid} for t, iid in local_only],
        "remote_only": [{"item_type": t, "item_id": iid} for t, iid in remote_only],
        "changed": [{"item_type": t, "item_id": iid} for t, iid in changed],
    }

    print("[sync-diff] summary:")
    for k, v in report["summary"].items():
        print(f"[sync-diff]   {k}: {v}")

    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"[sync-diff] wrote: {out.resolve()}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
