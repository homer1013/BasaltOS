#!/usr/bin/env python3
"""
BasaltOS configurator (v1.2)

Features:
- Discovers modules from: modules/*/module.json
- Discovers boards from:  boards/<platform>/**/board.json
- Board selection supports:
    * --board <board_dir> (folder name containing board.json)
    * --board <board_id>  (board.json "id") if unique; prompts if ambiguous in wizard
- Resolves module dependencies and conflicts
- Generates:
    config/generated/basalt_config.h
    config/generated/basalt.features.json
    config/generated/sdkconfig.defaults

Wizard mode:
- Choose platform -> choose board -> choose modules (filtered by board capabilities)
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Set, Optional, Tuple


# -----------------------------
# Data model
# -----------------------------

@dataclass(frozen=True)
class Module:
    id: str
    name: str
    provides: Tuple[str, ...]
    depends: Tuple[str, ...]
    conflicts: Tuple[str, ...]
    defines: Tuple[str, ...]
    description: str = ""
    raw: dict = None


@dataclass(frozen=True)
class BoardProfile:
    platform: str
    board_dir: str          # folder name under boards/<platform>/... containing board.json
    id: str                 # board.json "id" (family/alias)
    name: str               # board.json "name"
    target: str             # board.json "target" (optional)
    path: Path              # path to board.json
    data: dict              # parsed board.json


# -----------------------------
# Helpers
# -----------------------------

def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def norm_list_csv(value: Optional[str]) -> List[str]:
    if not value:
        return []
    parts: List[str] = []
    for chunk in value.split(","):
        s = chunk.strip()
        if s:
            parts.append(s)
    return parts


def eprint(msg: str) -> None:
    print(msg, file=sys.stderr)


def slug_to_macro(name: str) -> str:
    out: List[str] = []
    for ch in name:
        if ch.isalnum():
            out.append(ch.upper())
        else:
            out.append("_")
    macro = "".join(out)
    while "__" in macro:
        macro = macro.replace("__", "_")
    return macro.strip("_")


def safe_rel(root: Path, p: Path) -> str:
    try:
        return p.relative_to(root).as_posix()
    except Exception:
        return p.as_posix()


def prompt_choice(title: str, options: List[str], allow_empty: bool = False) -> Optional[str]:
    """
    Show a numbered list and return the selected option string.
    """
    print("")
    print(title)
    print("-" * len(title))
    if not options:
        print("(none)")
        return None

    for i, opt in enumerate(options, start=1):
        print(f"  {i:2d}) {opt}")

    if allow_empty:
        print("  0) (skip)")

    while True:
        raw = input("Choose: ").strip()
        if allow_empty and raw == "0":
            return None
        if raw.isdigit():
            idx = int(raw)
            if 1 <= idx <= len(options):
                return options[idx - 1]
        print(f"Invalid selection. Enter 1..{len(options)}" + (" or 0" if allow_empty else "") + ".")


# -----------------------------
# Load modules
# -----------------------------

def discover_modules(modules_dir: Path) -> Dict[str, Module]:
    if not modules_dir.exists():
        raise FileNotFoundError(f"modules directory not found: {modules_dir}")

    modules: Dict[str, Module] = {}
    for module_json in modules_dir.rglob("module.json"):
        data = load_json(module_json)

        required = ["id", "name", "provides", "depends", "conflicts", "defines"]
        missing = [k for k in required if k not in data]
        if missing:
            raise ValueError(f"{module_json}: missing keys: {missing}")

        mid = str(data["id"]).strip()
        if not mid:
            raise ValueError(f"{module_json}: invalid 'id'")

        if mid in modules:
            raise ValueError(f"duplicate module id '{mid}' found at {module_json}")

        modules[mid] = Module(
            id=mid,
            name=str(data.get("name", mid)),
            provides=tuple(data.get("provides", [])),
            depends=tuple(data.get("depends", [])),
            conflicts=tuple(data.get("conflicts", [])),
            defines=tuple(data.get("defines", [])),
            description=str(data.get("description", "")),
            raw=data,
        )

    if not modules:
        raise ValueError(f"no module.json files found under: {modules_dir}")

    return modules


# -----------------------------
# Board discovery + selection
# -----------------------------

def discover_boards(root: Path, platform: Optional[str]) -> List[BoardProfile]:
    boards_root = root / "boards"
    if not boards_root.exists():
        return []

    platforms: List[str]
    if platform:
        platforms = [platform]
    else:
        platforms = [p.name for p in boards_root.iterdir() if p.is_dir()]

    found: List[BoardProfile] = []
    for plat in platforms:
        plat_dir = boards_root / plat
        if not plat_dir.exists():
            continue

        for board_json in plat_dir.rglob("board.json"):
            try:
                data = load_json(board_json)
            except Exception as ex:
                eprint(f"[configure] WARNING: failed to parse {safe_rel(root, board_json)}: {ex}")
                continue

            board_dir = board_json.parent.name
            bid = str(data.get("id", "")).strip() or board_dir
            name = str(data.get("name", board_dir)).strip()
            target = str(data.get("target", "")).strip()

            found.append(BoardProfile(
                platform=plat,
                board_dir=board_dir,
                id=bid,
                name=name,
                target=target,
                path=board_json,
                data=data,
            ))

    return found


def board_capabilities(board_data: Optional[dict]) -> Optional[Set[str]]:
    if not board_data:
        return None
    caps = board_data.get("capabilities")
    if not isinstance(caps, list):
        return None
    return {str(c).strip() for c in caps if str(c).strip()}


class ConfigError(Exception):
    pass


def select_board_noninteractive(
    boards: List[BoardProfile],
    platform: Optional[str],
    board_arg: Optional[str],
) -> Tuple[Optional[BoardProfile], Optional[Set[str]]]:
    if not platform or not board_arg:
        return None, None

    board_arg = board_arg.strip()
    if not board_arg:
        return None, None

    candidates = [b for b in boards if b.platform == platform]
    if not candidates:
        return None, None

    # Prefer exact folder name match
    exact_dir = [b for b in candidates if b.board_dir == board_arg]
    if len(exact_dir) == 1:
        b = exact_dir[0]
        return b, board_capabilities(b.data)

    # Then try id match
    id_matches = [b for b in candidates if b.id == board_arg]
    if len(id_matches) == 1:
        b = id_matches[0]
        return b, board_capabilities(b.data)

    if len(id_matches) > 1:
        lines = [f"{b.board_dir} (id='{b.id}', name='{b.name}')" for b in sorted(id_matches, key=lambda x: x.board_dir)]
        raise ConfigError(
            "ambiguous board id; matches multiple variants:\n  - " + "\n  - ".join(lines)
        )

    return None, None


def wizard_select_platform(root: Path) -> Optional[str]:
    boards_root = root / "boards"
    if not boards_root.exists():
        print("[wizard] No boards/ directory found; skipping board selection.")
        return None
    plats = sorted([p.name for p in boards_root.iterdir() if p.is_dir()])
    if not plats:
        print("[wizard] No platforms found under boards/.")
        return None
    return prompt_choice("Select platform", plats)


def wizard_select_board(boards: List[BoardProfile], platform: str) -> Optional[BoardProfile]:
    plat_boards = [b for b in boards if b.platform == platform]
    if not plat_boards:
        print(f"[wizard] No boards found under platform '{platform}'.")
        return None

    # Present a clean list. Show board_dir as the canonical pick, but display id/name.
    display: List[str] = []
    index: Dict[str, BoardProfile] = {}
    for b in sorted(plat_boards, key=lambda x: x.board_dir):
        caps = b.data.get("capabilities", [])
        caps_s = ",".join(caps) if isinstance(caps, list) else ""
        line = f"{b.board_dir}   (id='{b.id}', name='{b.name}', caps=[{caps_s}])"
        display.append(line)
        index[line] = b

    choice = prompt_choice("Select board", display, allow_empty=True)
    if choice is None:
        return None
    return index[choice]


# -----------------------------
# Dependency resolution
# -----------------------------

def resolve_modules(
    modules: Dict[str, Module],
    requested: Set[str],
    allow: Optional[Set[str]] = None
) -> List[str]:
    unknown = sorted([m for m in requested if m not in modules])
    if unknown:
        raise ConfigError(f"Unknown module(s): {', '.join(unknown)}")

    enabled: Set[str] = set()
    stack: List[str] = sorted(requested)

    while stack:
        mid = stack.pop()
        if mid in enabled:
            continue

        if allow is not None and mid not in allow:
            raise ConfigError(f"Module '{mid}' is not supported by selected board/capabilities.")

        enabled.add(mid)

        for dep in modules[mid].depends:
            dep = dep.strip()
            if not dep:
                continue
            if dep not in modules:
                raise ConfigError(f"Module '{mid}' depends on missing module '{dep}'.")
            if allow is not None and dep not in allow:
                raise ConfigError(f"Module '{mid}' depends on '{dep}', but '{dep}' is not supported by the selected board.")
            if dep not in enabled:
                stack.append(dep)

    for mid in enabled:
        for c in modules[mid].conflicts:
            c = c.strip()
            if c and c in enabled:
                raise ConfigError(f"Conflict: '{mid}' conflicts with '{c}' (both enabled).")

    # topo-ish stable order: deps first, otherwise alpha
    output: List[str] = []
    remaining = set(enabled)

    while remaining:
        progressed = False
        for mid in sorted(list(remaining)):
            deps = set(d for d in modules[mid].depends if d.strip())
            if deps.issubset(set(output)):
                output.append(mid)
                remaining.remove(mid)
                progressed = True
                break
        if not progressed:
            cycle = ", ".join(sorted(remaining))
            raise ConfigError(f"Dependency cycle or unresolved deps among: {cycle}")

    return output


# -----------------------------
# Emit outputs
# -----------------------------

def emit_basalt_config_h(
    out_path: Path,
    enabled_modules: List[str],
    modules: Dict[str, Module],
    platform: Optional[str],
    board_define_name: Optional[str],
    board_data: Optional[dict],
) -> None:
    enabled = set(enabled_modules)

    # all defines across all modules, so disabled modules get explicit 0
    all_defines: List[str] = []
    for mid in sorted(modules.keys()):
        for d in modules[mid].defines:
            ds = str(d).strip()
            if ds and ds not in all_defines:
                all_defines.append(ds)

    define_enabled: Set[str] = set()
    for mid in enabled:
        for d in modules[mid].defines:
            ds = str(d).strip()
            if ds:
                define_enabled.add(ds)

    lines: List[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("/* Auto-generated by tools/configure.py")
    lines.append(" * DO NOT EDIT BY HAND")
    lines.append(" */")
    lines.append("")

    if platform:
        lines.append(f"#define BASALT_PLATFORM_{slug_to_macro(platform)} 1")
    if board_define_name:
        lines.append(f"#define BASALT_BOARD_{slug_to_macro(board_define_name)} 1")
    if platform or board_define_name:
        lines.append("")

    lines.append("/* Module enable flags */")
    for d in all_defines:
        lines.append(f"#define {d} {1 if d in define_enabled else 0}")
    lines.append("")

    if board_data and isinstance(board_data.get("pins"), dict):
        lines.append("/* Board pin bindings */")
        pins = board_data["pins"]
        for key in sorted(pins.keys()):
            macro = "BASALT_PIN_" + slug_to_macro(str(key))
            val = pins[key]
            if isinstance(val, (int, float)) and int(val) == val:
                lines.append(f"#define {macro} {int(val)}")
            else:
                lines.append(f'#define {macro} "{val}"')
        lines.append("")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def emit_features_json(
    out_path: Path,
    enabled_modules: List[str],
    modules: Dict[str, Module],
    platform: Optional[str],
    board_id: Optional[str],
    board_dir: Optional[str],
) -> None:
    provides: Set[str] = set()
    for mid in enabled_modules:
        for p in modules[mid].provides:
            ps = str(p).strip()
            if ps:
                provides.add(ps)

    payload = {
        "generated_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "platform": platform or "",
        "board_id": board_id or "",
        "board_dir": board_dir or "",
        "modules": enabled_modules,
        "provides": sorted(provides),
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def emit_sdkconfig_defaults(
    out_path: Path,
    root: Path,
    platform: Optional[str],
    board_profile: Optional[BoardProfile],
) -> None:
    parts: List[Tuple[str, Path]] = []

    if platform:
        p = root / "platforms" / platform / "idf_defaults.sdkconfig"
        if p.exists():
            parts.append((f"platform defaults: {safe_rel(root, p)}", p))

    if board_profile:
        b = board_profile.path.parent / "sdkconfig.defaults"
        if b.exists():
            parts.append((f"board defaults: {safe_rel(root, b)}", b))

    if not parts:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(
            "# Auto-generated by tools/configure.py\n"
            "# No platform/board sdkconfig defaults found.\n",
            encoding="utf-8",
        )
        return

    out_lines: List[str] = []
    out_lines.append("# Auto-generated by tools/configure.py")
    out_lines.append("# Concatenated defaults below")
    out_lines.append("")

    for label, path in parts:
        out_lines.append(f"# --- BEGIN {label} ---")
        out_lines.append(path.read_text(encoding="utf-8", errors="replace").rstrip())
        out_lines.append(f"# --- END {label} ---")
        out_lines.append("")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(out_lines).rstrip() + "\n", encoding="utf-8")


# -----------------------------
# Wizard: module selection
# -----------------------------

def wizard_select_modules(modules: Dict[str, Module], allowed: Optional[Set[str]]) -> Set[str]:
    candidates = sorted(modules.keys())
    if allowed is not None:
        candidates = [m for m in candidates if m in allowed]

    print("")
    print("Select modules to enable")
    print("------------------------")
    print("Enter comma-separated ids (e.g., spi,uart,tft)")
    print("Dependencies will be added automatically.")
    print("Conflicts will be rejected.")
    print("")

    for mid in candidates:
        m = modules[mid]
        dep = f" deps=[{','.join(m.depends)}]" if m.depends else ""
        conf = f" conflicts=[{','.join(m.conflicts)}]" if m.conflicts else ""
        print(f"  - {mid:10s} : {m.name}{dep}{conf}")
        if m.description.strip():
            print(f"      {m.description.strip()}")

    print("")
    choice = input("Enable modules (comma separated): ").strip()
    return set(norm_list_csv(choice))


def run_wizard(root: Path, modules: Dict[str, Module]) -> Tuple[Optional[str], Optional[BoardProfile], Set[str]]:
    """
    Returns (platform, board_profile, requested_modules)
    """
    print("")
    print("BasaltOS Configure Wizard")
    print("=========================")

    platform = wizard_select_platform(root)
    boards = discover_boards(root, platform)

    board_profile: Optional[BoardProfile] = None
    allow: Optional[Set[str]] = None

    if platform:
        board_profile = wizard_select_board(boards, platform)
        if board_profile:
            allow = board_capabilities(board_profile.data)

    requested = wizard_select_modules(modules, allow)
    return platform, board_profile, requested


# -----------------------------
# CLI
# -----------------------------

def main() -> int:
    root = repo_root()
    modules_dir = root / "modules"
    gen_dir = root / "config" / "generated"

    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("--platform", default=None, help="platform id (e.g., esp32)")
    ap.add_argument("--board", default=None, help="board dir OR board id (board.json 'id')")
    ap.add_argument("--enable", default=None, help="comma-separated module ids to enable")
    ap.add_argument("--disable", default=None, help="comma-separated module ids to disable (applied after enable/deps)")
    ap.add_argument("--wizard", action="store_true", help="interactive: choose platform, board, modules")
    ap.add_argument("--list-modules", action="store_true", help="list available modules")
    ap.add_argument("--list-boards", action="store_true", help="list available boards (filtered by --platform if set)")
    ap.add_argument("--outdir", default=str(gen_dir), help="output directory (default: config/generated)")
    args = ap.parse_args()

    try:
        modules = discover_modules(modules_dir)
    except Exception as ex:
        eprint(f"[configure] ERROR: {ex}")
        return 2

    boards = discover_boards(root, args.platform)

    if args.list_modules:
        for mid in sorted(modules.keys()):
            m = modules[mid]
            deps = ",".join(m.depends) if m.depends else ""
            conf = ",".join(m.conflicts) if m.conflicts else ""
            prov = ",".join(m.provides) if m.provides else ""
            defs = ",".join(m.defines) if m.defines else ""
            print(f"{mid}\n  name: {m.name}\n  provides: {prov}\n  depends: {deps}\n  conflicts: {conf}\n  defines: {defs}\n")
        return 0

    if args.list_boards:
        if not boards:
            print("(no boards found)")
            return 0

        by_plat: Dict[str, List[BoardProfile]] = {}
        for b in boards:
            by_plat.setdefault(b.platform, []).append(b)

        for plat in sorted(by_plat.keys()):
            print(f"{plat}:")
            for b in sorted(by_plat[plat], key=lambda x: x.board_dir):
                caps = b.data.get("capabilities", [])
                caps_s = ",".join(caps) if isinstance(caps, list) else ""
                print(f"  - {b.board_dir:20s}  id='{b.id}'  name='{b.name}'  target='{b.target}'  caps=[{caps_s}]")
            print("")
        return 0

    board_profile: Optional[BoardProfile] = None
    allow: Optional[Set[str]] = None
    platform: Optional[str] = args.platform
    requested: Set[str] = set(norm_list_csv(args.enable))
    disabled: Set[str] = set(norm_list_csv(args.disable))

    if args.wizard:
        platform, board_profile, requested = run_wizard(root, modules)
        allow = board_capabilities(board_profile.data) if board_profile else None
    else:
        # Non-wizard: honor flags
        if args.platform and args.board:
            try:
                board_profile, allow = select_board_noninteractive(boards, args.platform, args.board)
            except ConfigError as ex:
                eprint(f"[configure] ERROR: {ex}")
                eprint("[configure] TIP: run: python tools/configure.py --list-boards --platform <platform>")
                return 3

            if board_profile is None:
                eprint(f"[configure] WARNING: board '{args.board}' not found under platform '{args.platform}'.")
                eprint("[configure] TIP: run: python tools/configure.py --list-boards --platform <platform>")
                # continue module-only

        if not requested:
            requested = set()

    # Resolve modules
    try:
        enabled_list = resolve_modules(modules, requested, allow=allow)
    except ConfigError as ex:
        eprint(f"[configure] ERROR: {ex}")
        if allow is not None:
            eprint("[configure] NOTE: board capabilities restriction is active.")
            eprint(f"[configure] Allowed modules for board: {', '.join(sorted(allow))}")
        return 4

    # Apply disables after dependency resolution
    if disabled:
        enabled_set = set(enabled_list)
        for d in disabled:
            enabled_set.discard(d)

        # validate deps still satisfied
        for mid in list(enabled_set):
            deps = set(modules[mid].depends)
            if not deps.issubset(enabled_set):
                missing = ", ".join(sorted(deps - enabled_set))
                eprint(f"[configure] ERROR: disabling breaks dependencies: '{mid}' missing deps: {missing}")
                return 5

        # validate conflicts still ok
        for mid in enabled_set:
            for c in modules[mid].conflicts:
                if c in enabled_set:
                    eprint(f"[configure] ERROR: conflict after disable step: '{mid}' conflicts with '{c}'")
                    return 5

        enabled_list = resolve_modules(modules, enabled_set, allow=allow)

    outdir = Path(args.outdir).resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    basalt_h = outdir / "basalt_config.h"
    features_j = outdir / "basalt.features.json"
    sdk_defaults = outdir / "sdkconfig.defaults"

    board_define_name = None
    board_id = None
    board_dir = None
    board_data = None

    if board_profile:
        board_define_name = board_profile.id  # family name for macro
        board_id = board_profile.id
        board_dir = board_profile.board_dir
        board_data = board_profile.data
    else:
        # If user passed --board without finding a profile, still set the macro (best-effort)
        board_define_name = args.board

    emit_basalt_config_h(
        out_path=basalt_h,
        enabled_modules=enabled_list,
        modules=modules,
        platform=platform,
        board_define_name=board_define_name,
        board_data=board_data,
    )
    emit_features_json(
        out_path=features_j,
        enabled_modules=enabled_list,
        modules=modules,
        platform=platform,
        board_id=board_id,
        board_dir=board_dir,
    )
    emit_sdkconfig_defaults(
        out_path=sdk_defaults,
        root=root,
        platform=platform,
        board_profile=board_profile,
    )

    print("")
    print("[configure] Generated:")
    print(f"  - {basalt_h.relative_to(root)}")
    print(f"  - {features_j.relative_to(root)}")
    print(f"  - {sdk_defaults.relative_to(root)}")
    if board_profile:
        print(f"[configure] Board profile: {safe_rel(root, board_profile.path)}")

    print("")
    print("[configure] Next steps (ESP-IDF):")
    print("  1) Ensure your build includes config/generated in include paths")
    print("  2) Apply defaults when building, e.g.:")
    print("     SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build")
    print("")
    if enabled_list:
        print(f"[configure] Enabled modules: {', '.join(enabled_list)}")
    else:
        print("[configure] Enabled modules: (none)")
    print("")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
