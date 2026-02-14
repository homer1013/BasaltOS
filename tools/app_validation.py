#!/usr/bin/env python3
"""Shared validation helpers for Basalt app folders and packages."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Dict
import zipfile
import tempfile
import io

try:
    import tomllib  # Python 3.11+
except Exception:  # pragma: no cover
    tomllib = None


def _safe_rel(path: Path, root: Path) -> str:
    try:
        return str(path.resolve().relative_to(root.resolve()))
    except Exception:
        return str(path)


def parse_app_toml(app_toml: Path) -> Dict[str, Any]:
    if not app_toml.exists():
        return {}
    if tomllib is None:
        raise ValueError("Python tomllib is unavailable; cannot parse app.toml")
    try:
        data = tomllib.loads(app_toml.read_text(encoding="utf-8"))
    except Exception as e:
        raise ValueError(f"invalid app.toml: {e}") from e
    if not isinstance(data, dict):
        raise ValueError("invalid app.toml: top-level must be a table/object")
    return data


def _normalize_runtime(raw_runtime: Any) -> str:
    runtime = str(raw_runtime or "python").strip().lower()
    if runtime == "micropython":
        runtime = "python"
    if runtime not in {"python", "lua"}:
        raise ValueError(f"unsupported runtime '{runtime}' (supported: python, lua)")
    return runtime


def _default_entry_for_runtime(runtime: str) -> str:
    return "main.lua" if runtime == "lua" else "main.py"


def _validate_entry(app_root: Path, manifest: Dict[str, Any], runtime: str, has_manifest: bool) -> str:
    raw_entry = str(manifest.get("entry") or "").strip()
    if not raw_entry:
        raw_entry = _default_entry_for_runtime(runtime)
        if not has_manifest and runtime == "python":
            # Backward-compatible fallback for apps without app.toml.
            if not (app_root / raw_entry).is_file() and (app_root / "main.lua").is_file():
                raw_entry = "main.lua"
    if not raw_entry:
        raise ValueError("app.toml entry must not be empty")
    entry = Path(raw_entry)
    if entry.is_absolute() or ".." in entry.parts:
        raise ValueError("app.toml entry must be a safe relative path")
    if not (app_root / entry).is_file():
        raise ValueError(f"entry file missing: {raw_entry}")
    return raw_entry


def validate_app_dir(app_root: Path, *, check_py_syntax: bool = False) -> Dict[str, Any]:
    """Validate an app directory and return normalized metadata."""
    app_root = app_root.resolve()
    if not app_root.exists() or not app_root.is_dir():
        raise ValueError(f"app directory not found: {app_root}")

    app_toml = app_root / "app.toml"
    has_manifest = app_toml.exists()
    manifest = parse_app_toml(app_toml) if has_manifest else {}
    runtime = _normalize_runtime(manifest.get("runtime", "python"))
    entry = _validate_entry(app_root, manifest if has_manifest else {}, runtime, has_manifest)

    if check_py_syntax:
        import py_compile

        for py in sorted(app_root.rglob("*.py")):
            try:
                py_compile.compile(str(py), doraise=True)
            except Exception as e:
                rel = _safe_rel(py, app_root)
                raise ValueError(f"python syntax check failed for {rel}: {e}") from e

    name = str(manifest.get("name") or app_root.name).strip()
    version = str(manifest.get("version") or "0.1.0").strip()
    author = str(manifest.get("author") or "").strip()
    description = str(manifest.get("description") or "").strip()

    if not name:
        raise ValueError("app name must not be empty")
    if not version:
        raise ValueError("app version must not be empty")

    file_count = sum(1 for p in app_root.rglob("*") if p.is_file())
    return {
        "name": name,
        "version": version,
        "author": author,
        "description": description,
        "entry": entry,
        "runtime": runtime,
        "has_app_toml": app_toml.exists(),
        "file_count": file_count,
    }


def validate_zip_package(zip_bytes: bytes, *, check_py_syntax: bool = False) -> Dict[str, Any]:
    """Validate a zip payload and return normalized metadata."""
    if not zip_bytes:
        raise ValueError("uploaded package is empty")

    # Actual extraction and validation.
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        unpack = work / "unpack"
        unpack.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(io.BytesIO(zip_bytes)) as zf:
            names = zf.namelist()
            if not names:
                raise ValueError("ZIP package has no files")
            for name in names:
                p = Path(name)
                if p.is_absolute() or ".." in p.parts:
                    raise ValueError("ZIP contains unsafe paths")
            zf.extractall(unpack)

        children = [p for p in unpack.iterdir()]
        src = unpack
        if len(children) == 1 and children[0].is_dir():
            src = children[0]

        meta = validate_app_dir(src, check_py_syntax=check_py_syntax)
        return {"meta": meta}
