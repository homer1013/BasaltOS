#!/usr/bin/env python3
"""Validate BasaltOS board/module metadata files.

This script enforces a lightweight schema for:
- modules/*/module.json
- boards/<platform>/<board_dir>/board.json

It also performs cross-file consistency checks and can emit a remediation report.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


@dataclass
class ValidationError:
    path: Path
    message: str


_ID_RE = re.compile(r"^[a-z0-9_-]+$")


class Validator:
    def __init__(self, repo_root: Path) -> None:
        self.repo_root = repo_root
        self.errors: list[ValidationError] = []
        self.module_ids: set[str] = set()
        self.modules: dict[str, dict[str, Any]] = {}
        self.module_paths: dict[str, Path] = {}
        self.board_defaults_modules: list[tuple[Path, str]] = []

    def err(self, path: Path, message: str) -> None:
        self.errors.append(ValidationError(path=path, message=message))

    def load_json(self, path: Path) -> dict[str, Any] | None:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except Exception as ex:
            self.err(path, f"invalid JSON: {ex}")
            return None
        if not isinstance(data, dict):
            self.err(path, "top-level JSON value must be an object")
            return None
        return data

    def expect_str(self, path: Path, data: dict[str, Any], key: str, required: bool = True) -> str | None:
        if key not in data:
            if required:
                self.err(path, f"missing required key '{key}'")
            return None
        v = data[key]
        if not isinstance(v, str) or not v.strip():
            self.err(path, f"'{key}' must be a non-empty string")
            return None
        return v.strip()

    def expect_list_of_str(
        self, path: Path, data: dict[str, Any], key: str, required: bool = True
    ) -> list[str] | None:
        if key not in data:
            if required:
                self.err(path, f"missing required key '{key}'")
            return None
        v = data[key]
        if not isinstance(v, list):
            self.err(path, f"'{key}' must be a list")
            return None
        out: list[str] = []
        for i, item in enumerate(v):
            if not isinstance(item, str) or not item.strip():
                self.err(path, f"'{key}[{i}]' must be a non-empty string")
                continue
            out.append(item.strip())
        return out

    @staticmethod
    def _has_duplicates(items: list[str]) -> bool:
        return len(items) != len(set(items))

    def validate_module(self, path: Path) -> None:
        data = self.load_json(path)
        if data is None:
            return

        mid = self.expect_str(path, data, "id")
        self.expect_str(path, data, "name")
        provides = self.expect_list_of_str(path, data, "provides")
        deps = self.expect_list_of_str(path, data, "depends")
        conflicts = self.expect_list_of_str(path, data, "conflicts")
        defines = self.expect_list_of_str(path, data, "defines")

        if mid:
            if not _ID_RE.match(mid):
                self.err(path, "module 'id' must match ^[a-z0-9_-]+$")
            if mid in self.module_ids:
                self.err(path, f"duplicate module id '{mid}'")
            else:
                self.module_ids.add(mid)
                self.modules[mid] = data
                self.module_paths[mid] = path

        if provides is not None and self._has_duplicates(provides):
            self.err(path, "'provides' must not contain duplicates")
        if defines is not None and self._has_duplicates(defines):
            self.err(path, "'defines' must not contain duplicates")
        if deps is not None and self._has_duplicates(deps):
            self.err(path, "'depends' must not contain duplicates")
        if conflicts is not None and self._has_duplicates(conflicts):
            self.err(path, "'conflicts' must not contain duplicates")

        if deps and mid and mid in deps:
            self.err(path, f"module '{mid}' cannot depend on itself")
        if conflicts and mid and mid in conflicts:
            self.err(path, f"module '{mid}' cannot conflict with itself")
        if deps and conflicts and set(deps) & set(conflicts):
            overlap = ", ".join(sorted(set(deps) & set(conflicts)))
            self.err(path, f"'depends' and 'conflicts' overlap: {overlap}")

        platforms = data.get("platforms")
        if platforms is not None:
            if not isinstance(platforms, list):
                self.err(path, "'platforms' must be a list when present")
            else:
                for i, item in enumerate(platforms):
                    if not isinstance(item, str) or not item.strip():
                        self.err(path, f"'platforms[{i}]' must be a non-empty string")

        default_enabled = data.get("default_enabled")
        if default_enabled is not None and not isinstance(default_enabled, bool):
            self.err(path, "'default_enabled' must be a boolean when present")

        pins = data.get("pins")
        if pins is not None:
            if not isinstance(pins, list):
                self.err(path, "'pins' must be a list when present")
            else:
                pin_items: list[str] = []
                for i, item in enumerate(pins):
                    if not isinstance(item, str) or not item.strip():
                        self.err(path, f"'pins[{i}]' must be a non-empty string")
                    else:
                        pin_items.append(item.strip())
                if self._has_duplicates(pin_items):
                    self.err(path, "'pins' must not contain duplicates")

        pin_reqs = data.get("pin_requirements")
        if pin_reqs is not None:
            if not isinstance(pin_reqs, dict):
                self.err(path, "'pin_requirements' must be an object when present")
            else:
                for req_key, req_value in pin_reqs.items():
                    if not isinstance(req_key, str) or not req_key.strip():
                        self.err(path, "'pin_requirements' keys must be non-empty strings")
                        continue
                    if not isinstance(req_value, dict):
                        self.err(path, f"'pin_requirements.{req_key}' must be an object")
                        continue
                    req_required = req_value.get("required")
                    if req_required is not None and not isinstance(req_required, bool):
                        self.err(path, f"'pin_requirements.{req_key}.required' must be boolean when present")
                    req_count = req_value.get("count")
                    if req_count is not None and (not isinstance(req_count, int) or req_count < 1):
                        self.err(path, f"'pin_requirements.{req_key}.count' must be integer >= 1 when present")

        runtime = data.get("runtime")
        if runtime is not None and not isinstance(runtime, dict):
            self.err(path, "'runtime' must be an object when present")

    def validate_board(self, path: Path) -> None:
        data = self.load_json(path)
        if data is None:
            return

        rel = path.relative_to(self.repo_root)
        parts = rel.parts
        if len(parts) != 4 or parts[0] != "boards" or parts[3] != "board.json":
            self.err(path, "board.json must live at boards/<platform>/<board_dir>/board.json")
            return
        dir_platform = parts[1]

        bid = self.expect_str(path, data, "id")
        self.expect_str(path, data, "name")
        json_platform = self.expect_str(path, data, "platform")
        if json_platform and json_platform != dir_platform:
            self.err(path, f"'platform'='{json_platform}' must match directory platform '{dir_platform}'")

        if bid:
            if "/" in bid:
                self.err(path, "board 'id' must not contain '/'")
            if not _ID_RE.match(bid):
                self.err(path, "board 'id' must match ^[a-z0-9_-]+$")

        caps = self.expect_list_of_str(path, data, "capabilities")
        if caps is not None:
            if not caps:
                self.err(path, "'capabilities' must not be empty")
            elif self._has_duplicates(caps):
                self.err(path, "'capabilities' must not contain duplicates")

        if not (data.get("mcu") or data.get("soc") or data.get("target_profile")):
            self.err(path, "at least one of 'mcu', 'soc', or 'target_profile' is required")

        pins = data.get("pins")
        if not isinstance(pins, dict):
            self.err(path, "'pins' must be an object")
        elif not pins:
            self.err(path, "'pins' must not be empty")
        else:
            for k, v in pins.items():
                if not isinstance(k, str) or not k:
                    self.err(path, "all 'pins' keys must be non-empty strings")
                    continue
                if isinstance(v, int):
                    if v < -1:
                        self.err(path, f"pin '{k}' integer value must be >= -1")
                elif isinstance(v, str):
                    if not v.strip():
                        self.err(path, f"pin '{k}' string value must be non-empty")
                else:
                    self.err(path, f"pin '{k}' must be an integer or string pin label")

        pin_defs = data.get("pin_definitions")
        if pin_defs is not None and not isinstance(pin_defs, dict):
            self.err(path, "'pin_definitions' must be an object when present")

        defaults = data.get("defaults")
        if defaults is not None:
            if not isinstance(defaults, dict):
                self.err(path, "'defaults' must be an object when present")
            else:
                default_modules = defaults.get("modules")
                if default_modules is not None:
                    if not isinstance(default_modules, list):
                        self.err(path, "'defaults.modules' must be a list when present")
                    else:
                        for i, mid in enumerate(default_modules):
                            if not isinstance(mid, str) or not mid.strip():
                                self.err(path, f"'defaults.modules[{i}]' must be a non-empty string")
                            else:
                                self.board_defaults_modules.append((path, mid.strip()))

        if caps and "wifi" in caps and "uart" not in caps:
            self.err(path, "board with 'wifi' capability should also declare 'uart'")

    def cross_validate_modules(self) -> None:
        for mid, data in self.modules.items():
            module_path = self.module_paths.get(mid)
            if module_path is None:
                continue

            for dep in data.get("depends", []):
                if dep not in self.module_ids:
                    self.err(module_path, f"unknown dependency module '{dep}'")
            for conflict in data.get("conflicts", []):
                if conflict not in self.module_ids:
                    self.err(module_path, f"unknown conflict module '{conflict}'")

    def cross_validate_boards(self) -> None:
        for path, mid in self.board_defaults_modules:
            if mid not in self.module_ids:
                self.err(path, f"defaults.modules references unknown module '{mid}'")

    def _rel(self, path: Path) -> str:
        try:
            return str(path.relative_to(self.repo_root))
        except Exception:
            return str(path)

    def _remediation_hint(self, message: str) -> str:
        if "missing required key" in message:
            return "Add the required key with a valid value."
        if "must be a non-empty string" in message:
            return "Provide a non-empty string value."
        if "must be a list" in message:
            return "Replace the value with a JSON array."
        if "must be an object" in message:
            return "Replace the value with a JSON object."
        if "unknown dependency module" in message or "unknown conflict module" in message:
            return "Fix module references or add missing module metadata."
        if "defaults.modules references unknown module" in message:
            return "Update defaults.modules to valid module ids."
        if "at least one of 'mcu', 'soc', or 'target_profile' is required" in message:
            return "Add board silicon identity (prefer 'mcu' for MCU boards)."
        if "must not contain duplicates" in message:
            return "Deduplicate list items."
        if "must match ^[a-z0-9_-]+$" in message:
            return "Use lowercase ids with [a-z0-9_-]."
        return "Review schema expectations and correct the metadata entry."

    def write_report(self, report_path: Path, module_count: int, board_count: int) -> None:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        grouped: dict[str, list[str]] = defaultdict(list)
        for err in self.errors:
            grouped[self._rel(err.path)].append(err.message)

        now = datetime.now(timezone.utc).isoformat()
        lines: list[str] = []
        lines.append(f"# Metadata Validation Report ({now})")
        lines.append("")
        lines.append(f"- Modules scanned: {module_count}")
        lines.append(f"- Boards scanned: {board_count}")
        lines.append(f"- Error count: {len(self.errors)}")
        lines.append(f"- Files with errors: {len(grouped)}")
        lines.append("")

        if not self.errors:
            lines.append("## Result")
            lines.append("")
            lines.append("Validation passed with no metadata errors.")
            lines.append("")
        else:
            lines.append("## Remediation Backlog")
            lines.append("")
            for path in sorted(grouped.keys()):
                lines.append(f"### `{path}`")
                for msg in grouped[path]:
                    hint = self._remediation_hint(msg)
                    lines.append(f"- Error: {msg}")
                    lines.append(f"  - Fix: {hint}")
                lines.append("")

        report_path.write_text("\n".join(lines), encoding="utf-8")

    def run(self, report_path: Path | None = None) -> int:
        modules_dir = self.repo_root / "modules"
        boards_dir = self.repo_root / "boards"

        if not modules_dir.exists():
            self.err(modules_dir, "modules directory not found")
            return 1
        if not boards_dir.exists():
            self.err(boards_dir, "boards directory not found")
            return 1

        module_paths = sorted(modules_dir.rglob("module.json"))
        board_paths = sorted(boards_dir.rglob("board.json"))

        if not module_paths:
            self.err(modules_dir, "no module.json files found")
        if not board_paths:
            self.err(boards_dir, "no board.json files found")

        for path in module_paths:
            self.validate_module(path)
        for path in board_paths:
            self.validate_board(path)

        self.cross_validate_modules()
        self.cross_validate_boards()

        if report_path is not None:
            self.write_report(report_path, len(module_paths), len(board_paths))
            print(f"Wrote report: {report_path}")

        if self.errors:
            print(f"Metadata validation failed: {len(self.errors)} error(s)")
            for e in self.errors:
                print(f"- {self._rel(e.path)}: {e.message}")
            return 1

        print(f"Metadata validation passed: {len(module_paths)} modules, {len(board_paths)} boards")
        return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Validate BasaltOS board/module metadata")
    ap.add_argument(
        "--report",
        default=None,
        help="optional markdown report output path (e.g. tmp/metadata/validation_report.md)",
    )
    args = ap.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    report = Path(args.report).resolve() if args.report else None
    return Validator(repo_root).run(report_path=report)


if __name__ == "__main__":
    raise SystemExit(main())
