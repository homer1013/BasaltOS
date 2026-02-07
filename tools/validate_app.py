#!/usr/bin/env python3
"""Validate a Basalt app folder or zip package."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import zipfile

from app_validation import validate_app_dir, validate_zip_package


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a Basalt app folder or zip package")
    parser.add_argument("path", help="Path to app folder or zip package")
    parser.add_argument(
        "--check-syntax",
        action="store_true",
        help="Run CPython syntax checks on .py files (best-effort)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print validation output as JSON",
    )
    args = parser.parse_args()

    target = Path(args.path).resolve()
    if not target.exists():
        print(f"error: path not found: {target}")
        return 1

    try:
        if target.is_dir():
            meta = validate_app_dir(target, check_py_syntax=args.check_syntax)
            payload = {"type": "dir", "path": str(target), "meta": meta}
        elif target.is_file() and target.suffix.lower() == ".zip":
            raw = target.read_bytes()
            result = validate_zip_package(raw, check_py_syntax=args.check_syntax)
            payload = {"type": "zip", "path": str(target), "meta": result["meta"]}
        else:
            print("error: path must be an app folder or .zip file")
            return 1
    except (ValueError, zipfile.BadZipFile) as e:
        print(f"error: {e}")
        return 1

    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        meta = payload["meta"]
        print("ok: app validation passed")
        print(f"path: {payload['path']}")
        print(f"name: {meta.get('name')}")
        print(f"version: {meta.get('version')}")
        print(f"entry: {meta.get('entry')}")
        print(f"files: {meta.get('file_count')}")
        print(f"app.toml: {'yes' if meta.get('has_app_toml') else 'no'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
