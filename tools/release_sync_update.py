#!/usr/bin/env python3
"""Update docs/RELEASE_SYNC_STATUS.md in one command.

Example:
  python3 tools/release_sync_update.py --version v0.1.1 --all-status in_progress
  python3 tools/release_sync_update.py --version v0.1.1 --main-status synced --platform-status synced --platformio-status in_progress
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

VERSION_RE = re.compile(r"^v\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")

REPOS = [
    "BasaltOS",
    "BasaltOS_Platform",
    "BasaltOS_PlatformIO",
]


def parse_line(line: str) -> list[str] | None:
    t = line.strip()
    if not t.startswith("|"):
        return None
    parts = [p.strip() for p in t.strip("|").split("|")]
    if len(parts) < 4:
        return None
    return parts


def format_row(repo: str, release: str, status: str, notes: str) -> str:
    return f"| {repo} | {release} | {status} | {notes} |"


def main() -> int:
    ap = argparse.ArgumentParser(description="Update release sync status table")
    ap.add_argument("--version", required=True, help="Release tag/version (example: v0.1.0)")
    ap.add_argument("--status-file", default="docs/RELEASE_SYNC_STATUS.md", help="Path to release sync status markdown")
    ap.add_argument("--all-status", default="", help="Status to apply to all rows")
    ap.add_argument("--main-status", default="", help="Status override for BasaltOS")
    ap.add_argument("--platform-status", default="", help="Status override for BasaltOS_Platform")
    ap.add_argument("--platformio-status", default="", help="Status override for BasaltOS_PlatformIO")
    args = ap.parse_args()

    version = args.version.strip()
    if not VERSION_RE.match(version):
        raise SystemExit(f"Invalid --version: {version}")

    status_file = Path(args.status_file)
    if not status_file.exists():
        raise SystemExit(f"Missing status file: {status_file}")

    text = status_file.read_text(encoding="utf-8")
    lines = text.splitlines()

    defaults = {
        "BasaltOS": args.all_status.strip() or "synced",
        "BasaltOS_Platform": args.all_status.strip() or "synced",
        "BasaltOS_PlatformIO": args.all_status.strip() or "synced",
    }
    if args.main_status.strip():
        defaults["BasaltOS"] = args.main_status.strip()
    if args.platform_status.strip():
        defaults["BasaltOS_Platform"] = args.platform_status.strip()
    if args.platformio_status.strip():
        defaults["BasaltOS_PlatformIO"] = args.platformio_status.strip()

    found = {r: False for r in REPOS}
    out_lines: list[str] = []

    for line in lines:
        parts = parse_line(line)
        if not parts:
            out_lines.append(line)
            continue

        repo = parts[0]
        if repo in REPOS:
            notes = parts[3]
            out_lines.append(format_row(repo, version, defaults[repo], notes))
            found[repo] = True
        else:
            out_lines.append(line)

    if not all(found.values()):
        missing = [k for k, v in found.items() if not v]
        raise SystemExit(f"Missing repo rows in status file: {', '.join(missing)}")

    status_file.write_text("\n".join(out_lines) + "\n", encoding="utf-8")

    print(status_file)
    for repo in REPOS:
        print(f"{repo}: release={version} status={defaults[repo]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
