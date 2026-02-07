#!/usr/bin/env python3
"""Pack a Basalt OS app into a store-only zip compatible with on-device install."""

from __future__ import annotations

import argparse
import os
import sys
import time
import zipfile
from pathlib import Path

from app_validation import validate_app_dir


def _norm_slashes(path: str) -> str:
    return path.replace(os.sep, "/")


def _file_zipinfo(src_path: str, arc_path: str) -> zipfile.ZipInfo:
    st = os.stat(src_path)
    mtime = time.localtime(st.st_mtime)
    zi = zipfile.ZipInfo(filename=arc_path, date_time=mtime[:6])
    zi.compress_type = zipfile.ZIP_STORED
    # Regular file with 0o644 perms.
    zi.external_attr = 0o100644 << 16
    zi.flag_bits = 0
    return zi


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Pack a Basalt OS app into a store-only zip."
    )
    parser.add_argument("app_dir", help="Path to app folder")
    parser.add_argument("out_zip", help="Output zip path")
    parser.add_argument(
        "--name",
        help="Name of the app folder inside the zip (default: app_dir basename)",
        default=None,
    )
    parser.add_argument(
        "--check-syntax",
        action="store_true",
        help="Run CPython syntax checks on .py files before packing (best-effort)",
    )
    args = parser.parse_args()

    app_dir = os.path.abspath(args.app_dir)
    if not os.path.isdir(app_dir):
        print(f"error: app_dir not found: {app_dir}", file=sys.stderr)
        return 1

    app_name = args.name or os.path.basename(app_dir.rstrip(os.sep))
    if not app_name:
        print("error: app name is empty", file=sys.stderr)
        return 1

    try:
        validate_app_dir(
            Path(app_dir),
            check_py_syntax=args.check_syntax,
        )
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    files = []
    for root, _dirs, filenames in os.walk(app_dir):
        for name in filenames:
            src = os.path.join(root, name)
            rel = os.path.relpath(src, app_dir)
            rel = _norm_slashes(rel)
            if rel.startswith("../") or rel.startswith("/"):
                print(f"error: invalid path: {rel}", file=sys.stderr)
                return 1
            arc = _norm_slashes(os.path.join(app_name, rel))
            files.append((src, arc))

    out_zip = os.path.abspath(args.out_zip)
    os.makedirs(os.path.dirname(out_zip), exist_ok=True)

    with zipfile.ZipFile(out_zip, "w", compression=zipfile.ZIP_STORED) as zf:
        for src, arc in sorted(files):
            with open(src, "rb") as f:
                data = f.read()
            zi = _file_zipinfo(src, arc)
            zf.writestr(zi, data, compress_type=zipfile.ZIP_STORED)

    print(f"packed {len(files)} file(s) into {out_zip}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
