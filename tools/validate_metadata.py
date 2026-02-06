#!/usr/bin/env python3
"""Validate BasaltOS board/module metadata files.

This script enforces a lightweight schema for:
- modules/*/module.json
- boards/<platform>/<board_dir>/board.json

It also performs cross-file consistency checks.
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass
class ValidationError:
    path: Path
    message: str


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

    def validate_module(self, path: Path) -> None:
        data = self.load_json(path)
        if data is None:
            return

        mid = self.expect_str(path, data, "id")
        self.expect_str(path, data, "name")
        self.expect_list_of_str(path, data, "provides")
        deps = self.expect_list_of_str(path, data, "depends")
        conflicts = self.expect_list_of_str(path, data, "conflicts")
        self.expect_list_of_str(path, data, "defines")

        if mid:
            if mid in self.module_ids:
                self.err(path, f"duplicate module id '{mid}'")
            else:
                self.module_ids.add(mid)
                self.modules[mid] = data
                self.module_paths[mid] = path

        if deps and mid and mid in deps:
            self.err(path, f"module '{mid}' cannot depend on itself")
        if conflicts and mid and mid in conflicts:
            self.err(path, f"module '{mid}' cannot conflict with itself")

        platforms = data.get("platforms")
        if platforms is not None and not isinstance(platforms, list):
            self.err(path, "'platforms' must be a list when present")

        default_enabled = data.get("default_enabled")
        if default_enabled is not None and not isinstance(default_enabled, bool):
            self.err(path, "'default_enabled' must be a boolean when present")

        pins = data.get("pins")
        if pins is not None and not isinstance(pins, list):
            self.err(path, "'pins' must be a list when present")

        pin_reqs = data.get("pin_requirements")
        if pin_reqs is not None and not isinstance(pin_reqs, dict):
            self.err(path, "'pin_requirements' must be an object when present")

        runtime = data.get("runtime")
        if runtime is not None and not isinstance(runtime, dict):
            self.err(path, "'runtime' must be an object when present")

    def validate_board(self, path: Path) -> None:
        data = self.load_json(path)
        if data is None:
            return

        # Expect boards/<platform>/<board_dir>/board.json
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

        caps = self.expect_list_of_str(path, data, "capabilities")
        pins = data.get("pins")
        if not isinstance(pins, dict):
            self.err(path, "'pins' must be an object")
        else:
            for k, v in pins.items():
                if not isinstance(k, str) or not k:
                    self.err(path, "all 'pins' keys must be non-empty strings")
                    continue
                if not isinstance(v, (int, str)):
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

        if caps and "wifi" in caps:
            # Soft guard: boards declaring wifi should expose UART too for shell diagnostics.
            if "uart" not in caps:
                self.err(path, "board with 'wifi' capability should also declare 'uart'")

        if bid and "/" in bid:
            self.err(path, "board 'id' must not contain '/'")

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

    def run(self) -> int:
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

        if self.errors:
            print(f"Metadata validation failed: {len(self.errors)} error(s)")
            for e in self.errors:
                rel = e.path.relative_to(self.repo_root) if e.path.is_absolute() else e.path
                print(f"- {rel}: {e.message}")
            return 1

        print(
            f"Metadata validation passed: {len(module_paths)} modules, {len(board_paths)} boards"
        )
        return 0


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    return Validator(repo_root).run()


if __name__ == "__main__":
    raise SystemExit(main())
