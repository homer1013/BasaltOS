#!/usr/bin/env python3
"""
BasaltOS configurator (v1.2)

Features:
- Discovers drivers from: modules/*/module.json
- Discovers boards from:  boards/<platform>/**/board.json
- Board selection supports:
    * --board <board_dir> (folder name containing board.json)
    * --board <board_id>  (board.json "id") if unique; prompts if ambiguous in wizard
- Resolves driver dependencies and conflicts
- Generates:
    config/generated/basalt_config.h
    config/generated/basalt.features.json
    config/generated/sdkconfig.defaults

Wizard mode:
- Choose platform -> choose board -> choose drivers (filtered by board capabilities)
"""

from __future__ import annotations

import argparse
import difflib
import json
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Set, Optional, Tuple, Any
import re


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
    runtime: dict = None
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


@dataclass(frozen=True)
class ProjectTemplate:
    id: str
    name: str
    description: str
    platforms: Tuple[str, ...]
    enabled_drivers: Tuple[str, ...]
    applets: Tuple[str, ...]
    market_apps: Tuple[str, ...]
    driver_config: dict
    raw: dict


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


def parse_flash_mb(flash_str: str) -> Optional[int]:
    if not flash_str:
        return None
    m = re.search(r"(\d+)\s*MB", flash_str, re.IGNORECASE)
    if not m:
        return None
    try:
        return int(m.group(1))
    except Exception:
        return None


def norm_slug(value: str) -> str:
    return "".join(ch for ch in str(value).strip().lower() if ch.isalnum() or ch in ("_", "-")).replace("-", "_")


PLATFORM_MANUFACTURER = {
    "esp32": "Espressif",
    "avr": "Microchip",
    "atmega": "Microchip",
    "pic16": "Microchip",
    "rp2040": "Raspberry Pi",
    "stm32": "STMicroelectronics",
    "atsam": "Microchip",
    "renesas_ra": "Renesas",
    "renesas_rx": "Renesas",
    "linux-sbc": "Linux SBC",
}


PLATFORM_ARCHITECTURE = {
    "avr": "AVR 8-bit",
    "atmega": "AVR 8-bit",
    "pic16": "PIC 8-bit",
    "stm32": "ARM Cortex-M",
    "atsam": "ARM Cortex-M",
    "rp2040": "ARM Cortex-M0+",
    "renesas_ra": "ARM Cortex-M",
    "renesas_rx": "Renesas RX 32-bit",
    "linux-sbc": "Linux Host",
}


def mcu_family_label(mcu: str, platform: str) -> str:
    text = str(mcu or "").upper()
    plat = str(platform or "").lower()
    if plat == "esp32":
        if "C6" in text:
            return "ESP32-C6"
        if "C3" in text:
            return "ESP32-C3"
        if "S3" in text:
            return "ESP32-S3"
        if "S2" in text:
            return "ESP32-S2"
        return "ESP32"
    if plat == "stm32":
        m = re.search(r"STM32([A-Z]\d)", text)
        if m:
            return f"STM32{m.group(1)}"
        if "STM32" in text:
            return "STM32"
    if plat == "rp2040":
        return "RP2040"
    if plat == "atsam":
        if "SAMD21" in text:
            return "SAMD21"
        if "SAMD51" in text:
            return "SAMD51"
        return "SAM"
    if plat == "renesas_ra":
        if "RA4" in text:
            return "RA4"
        if "RA6" in text:
            return "RA6"
        return "Renesas RA"
    if plat == "renesas_rx":
        if "RX72" in text:
            return "RX72"
        if "RX65" in text:
            return "RX65"
        return "Renesas RX"
    if plat in {"avr", "atmega"}:
        if "ATMEGA32U4" in text:
            return "ATmega32U4"
        if "ATMEGA2560" in text:
            return "ATmega2560"
        if "ATMEGA328" in text:
            return "ATmega328"
        return "ATmega"
    if plat == "pic16":
        return "PIC16"
    return plat.upper() if plat else "General"


def board_taxonomy(board: BoardProfile) -> Dict[str, str]:
    data = board.data or {}
    platform = str(board.platform or "").lower()
    mcu = str(data.get("mcu") or data.get("soc") or data.get("target_profile") or "")
    mcu_up = mcu.upper()
    manufacturer = str(data.get("manufacturer") or PLATFORM_MANUFACTURER.get(platform) or "Generic")
    architecture = str(data.get("architecture") or PLATFORM_ARCHITECTURE.get(platform) or "Embedded")
    if platform == "esp32":
        architecture = "RISC-V" if ("C3" in mcu_up or "C6" in mcu_up) else "Xtensa"
    elif platform == "linux-sbc" and "BCM" in mcu_up:
        architecture = "ARM Cortex-A"
        manufacturer = "Raspberry Pi"
    family = str(data.get("family") or mcu_family_label(mcu, platform))
    silicon = mcu or str(data.get("target_profile") or data.get("id") or board.id or "Unknown")
    return {
        "manufacturer": manufacturer,
        "architecture": architecture,
        "family": family,
        "silicon": silicon,
    }


def read_json_file(path: Path, default: Any) -> Any:
    try:
        if path.exists():
            return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        pass
    return default


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


def prompt_yes_no(question: str, default: bool = True) -> bool:
    suffix = "[Y/n]" if default else "[y/N]"
    while True:
        raw = input(f"{question} {suffix}: ").strip().lower()
        if not raw:
            return default
        if raw in ("y", "yes"):
            return True
        if raw in ("n", "no"):
            return False
        print("Please answer y or n.")


def prompt_text(question: str, default: str = "") -> str:
    if default:
        raw = input(f"{question} [{default}]: ").strip()
        return raw if raw else default
    return input(f"{question}: ").strip()


def prompt_optional_choice(title: str, options: List[str], all_label: str) -> Optional[str]:
    if not options:
        return None
    labeled = [all_label] + options
    picked = prompt_choice(title, labeled)
    if picked == all_label:
        return None
    return picked


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
            runtime=dict(data.get("runtime", {})) if isinstance(data.get("runtime", {}), dict) else {},
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
    out = {str(c).strip() for c in caps if str(c).strip()}
    # Filesystem capability mapping.
    # SPIFFS is internal flash-backed and should be allowed on most boards.
    out.add("fs_spiffs")
    # If SD card hardware exists, allow SD filesystem module.
    if "sd_card" in out:
        out.add("fs_sd")
    # If UART is available, allow shell variants in the wizard.
    if "uart" in out:
        out.add("shell_full")
        out.add("shell_min")
        out.add("shell_tiny")
    # Driver-module convenience mapping based on transport capabilities.
    if "i2c" in out:
        out.update({"rtc", "imu", "display_ssd1306", "ads1115", "bme280"})
    if "spi" in out:
        out.add("mcp2515")
    if "gpio" in out:
        out.update({"dht22", "uln2003", "l298n"})
    if "adc" in out or "i2s" in out:
        out.add("mic")
    return out


class ConfigError(Exception):
    pass


def suggest_board_matches(
    boards: List[BoardProfile],
    platform: str,
    query: str,
    limit: int = 6,
) -> List[str]:
    q = str(query or "").strip().lower()
    if not q:
        return []
    candidates = [b for b in boards if b.platform == platform]
    if not candidates:
        return []

    out: List[str] = []
    # 1) direct substring hits first
    for b in sorted(candidates, key=lambda x: (x.board_dir, x.id)):
        if q in b.board_dir.lower() or q in b.id.lower() or q in b.name.lower():
            out.append(f"{b.board_dir} (id='{b.id}', name='{b.name}')")
            if len(out) >= limit:
                return out

    # 2) fuzzy fallback across dir/id/name tokens
    token_map: Dict[str, BoardProfile] = {}
    tokens: List[str] = []
    for b in candidates:
        for tok in (b.board_dir, b.id, b.name):
            key = str(tok).strip().lower()
            if not key:
                continue
            token_map[key] = b
            tokens.append(key)

    for tok in difflib.get_close_matches(q, sorted(set(tokens)), n=limit * 2, cutoff=0.45):
        b = token_map.get(tok)
        if not b:
            continue
        label = f"{b.board_dir} (id='{b.id}', name='{b.name}')"
        if label not in out:
            out.append(label)
            if len(out) >= limit:
                break
    return out


def suggest_driver_matches(modules: Dict[str, Module], query: str, limit: int = 6) -> List[str]:
    q = norm_slug(query or "")
    if not q:
        return []
    known = sorted(modules.keys())
    if q in known:
        return [q]
    matches = [m for m in known if q in m]
    if len(matches) < limit:
        fuzzy = difflib.get_close_matches(q, known, n=limit, cutoff=0.45)
        for m in fuzzy:
            if m not in matches:
                matches.append(m)
    return matches[:limit]


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
        known_platforms = sorted({b.platform for b in boards})
        raise ConfigError(
            f"platform '{platform}' has no discovered boards. "
            f"Known platforms: {', '.join(known_platforms) if known_platforms else '(none)'}"
        )

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

    hints = suggest_board_matches(boards, platform, board_arg)
    msg = f"board '{board_arg}' not found under platform '{platform}'."
    if hints:
        msg += "\nClosest matches:\n  - " + "\n  - ".join(hints)
    raise ConfigError(msg)


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


def wizard_select_board_hierarchy(boards: List[BoardProfile]) -> Optional[BoardProfile]:
    if not boards:
        print("[wizard] No boards found.")
        return None

    print("")
    print("Board Taxonomy Filters")
    print("----------------------")
    print("Tip: choose '(all ...)' at any step to keep filtering broad.")

    tx_pairs = [(b, board_taxonomy(b)) for b in boards]
    manufacturers = sorted({tx["manufacturer"] for _, tx in tx_pairs})
    selected_manufacturer = prompt_optional_choice("Manufacturer", manufacturers, "(all manufacturers)")

    by_manufacturer = [pair for pair in tx_pairs if not selected_manufacturer or pair[1]["manufacturer"] == selected_manufacturer]
    architectures = sorted({tx["architecture"] for _, tx in by_manufacturer})
    selected_architecture = prompt_optional_choice("Architecture", architectures, "(all architectures)")

    by_architecture = [pair for pair in by_manufacturer if not selected_architecture or pair[1]["architecture"] == selected_architecture]
    families = sorted({tx["family"] for _, tx in by_architecture})
    selected_family = prompt_optional_choice("Family", families, "(all families)")

    by_family = [pair for pair in by_architecture if not selected_family or pair[1]["family"] == selected_family]
    silicons = sorted({tx["silicon"] for _, tx in by_family})
    selected_silicon = prompt_optional_choice("Processor / Silicon", silicons, "(all processors)")

    filtered = [b for b, tx in by_family if not selected_silicon or tx["silicon"] == selected_silicon]
    if not filtered:
        print("[wizard] No boards match selected taxonomy filters.")
        return None

    display: List[str] = []
    index: Dict[str, BoardProfile] = {}
    for b in sorted(filtered, key=lambda x: (x.platform, x.board_dir)):
        caps = b.data.get("capabilities", [])
        caps_s = ",".join(caps) if isinstance(caps, list) else ""
        tx = board_taxonomy(b)
        line = f"{b.name} [{b.platform}/{b.board_dir}] ({tx['manufacturer']} • {tx['architecture']} • {tx['family']} • {tx['silicon']}; caps=[{caps_s}])"
        display.append(line)
        index[line] = b

    choice = prompt_choice("Select board", display, allow_empty=True)
    if choice is None:
        return None
    return index[choice]


# -----------------------------
# Templates + app catalogs
# -----------------------------

def discover_templates(root: Path) -> Dict[str, ProjectTemplate]:
    path = root / "config" / "templates" / "project_templates.json"
    data = read_json_file(path, {})
    items = data.get("templates", []) if isinstance(data, dict) else []
    out: Dict[str, ProjectTemplate] = {}
    for item in items:
        if not isinstance(item, dict):
            continue
        tid = norm_slug(item.get("id", ""))
        if not tid:
            continue
        platforms = tuple(str(x).strip() for x in item.get("platforms", []) if str(x).strip())
        enabled_drivers = tuple(str(x).strip() for x in item.get("enabled_drivers", []) if str(x).strip())
        applets = tuple(str(x).strip() for x in item.get("applets", []) if str(x).strip())
        market_apps = tuple(str(x).strip() for x in item.get("market_apps", []) if str(x).strip())
        driver_config = item.get("driver_config", {})
        if not isinstance(driver_config, dict):
            driver_config = {}
        out[tid] = ProjectTemplate(
            id=tid,
            name=str(item.get("name", tid)).strip(),
            description=str(item.get("description", "")).strip(),
            platforms=platforms,
            enabled_drivers=enabled_drivers,
            applets=applets,
            market_apps=market_apps,
            driver_config=driver_config,
            raw=item,
        )
    return out


def discover_market_app_ids(root: Path, platform: Optional[str]) -> Set[str]:
    path = root / "config" / "market_apps.json"
    items = read_json_file(path, [])
    out: Set[str] = set()
    if not isinstance(items, list):
        return out
    for item in items:
        if not isinstance(item, dict):
            continue
        aid = norm_slug(item.get("id", ""))
        if not aid:
            continue
        plats = item.get("platforms", [])
        if isinstance(plats, list) and plats and platform and platform not in {str(x).strip() for x in plats}:
            continue
        out.add(aid)
    return out


def template_allowed_for_platform(template: ProjectTemplate, platform: Optional[str]) -> bool:
    if not template.platforms:
        return True
    if not platform:
        return True
    return platform in set(template.platforms)


def known_applet_ids(root: Path) -> Set[str]:
    known = {
        "blink",
        "system_info",
        "tft_hello",
        "i2c_scan",
        "sd_probe",
        "fs_smoke",
        "remote_node",
        "uart_echo",
        "paint",
    }
    templates = discover_templates(root)
    for t in templates.values():
        for aid in t.applets:
            slug = norm_slug(aid)
            if slug:
                known.add(slug)
    return known


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
        details: List[str] = []
        for miss in unknown:
            hints = suggest_driver_matches(modules, miss)
            if hints:
                details.append(f"{miss} -> did you mean: {', '.join(hints)}")
        hint_block = ""
        if details:
            hint_block = "\nDriver suggestions:\n  - " + "\n  - ".join(details)
        sample = ", ".join(sorted(list(modules.keys()))[:12])
        raise ConfigError(
            f"Unknown driver(s): {', '.join(unknown)}"
            f"{hint_block}\nAvailable drivers (sample): {sample}"
        )

    enabled: Set[str] = set()
    stack: List[str] = sorted(requested)

    while stack:
        mid = stack.pop()
        if mid in enabled:
            continue

        if allow is not None and mid not in allow:
            raise ConfigError(
                f"Driver '{mid}' is not supported by selected board/capabilities. "
                "Run --list-boards for a different board, or choose from board-supported drivers."
            )

        enabled.add(mid)

        for dep in modules[mid].depends:
            dep = dep.strip()
            if not dep:
                continue
            if dep not in modules:
                raise ConfigError(f"Driver '{mid}' depends on missing driver '{dep}'.")
            if allow is not None and dep not in allow:
                raise ConfigError(
                    f"Driver '{mid}' depends on '{dep}', but '{dep}' is not supported by the selected board."
                )
            if dep not in enabled:
                stack.append(dep)

    for mid in enabled:
        for c in modules[mid].conflicts:
            c = c.strip()
            if not c:
                continue
            if {mid, c} == {"fs_spiffs", "fs_sd"}:
                continue
            if c in enabled:
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

    lines.append("/* Driver enable flags */")
    for d in all_defines:
        lines.append(f"#define {d} {1 if d in define_enabled else 0}")
    lines.append("")

    # Driver runtime settings (emitted only for enabled drivers)
    runtime_lines: List[str] = []
    for mid in enabled_modules:
        mod = modules.get(mid)
        if not mod or not mod.runtime:
            continue
        for key, val in mod.runtime.items():
            macro = f"BASALT_{slug_to_macro(mid)}_{slug_to_macro(str(key))}"
            if isinstance(val, bool):
                runtime_lines.append(f"#define {macro} {1 if val else 0}")
            elif isinstance(val, (int, float)) and int(val) == val:
                runtime_lines.append(f"#define {macro} {int(val)}")
            elif isinstance(val, (int, float)):
                runtime_lines.append(f"#define {macro} {val}")
            else:
                runtime_lines.append(f"#define {macro} \"{val}\"")

    if runtime_lines:
        lines.append("/* Driver runtime settings */")
        lines.extend(runtime_lines)
        lines.append("")

    if board_data and isinstance(board_data, dict):
        board_setting_lines: List[str] = []

        led_mode = str(board_data.get("led_mode", "")).strip().lower()
        if led_mode in {"single", "rgb"}:
            board_setting_lines.append(f"#define BASALT_LED_MODE_{slug_to_macro(led_mode)} 1")

        if "led_active_low" in board_data:
            led_active_low = bool(board_data.get("led_active_low"))
            board_setting_lines.append(f"#define BASALT_LED_ACTIVE_LOW {1 if led_active_low else 0}")

        if board_setting_lines:
            lines.append("/* Board-level settings */")
            lines.extend(board_setting_lines)
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
        "drivers": enabled_modules,
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
    enabled_modules: List[str],
    modules: Dict[str, Module],
) -> None:
    parts: List[Tuple[str, Path]] = []
    spiram_enabled_by_defaults = False

    if platform:
        p = root / "platforms" / platform / "idf_defaults.sdkconfig"
        if p.exists():
            parts.append((f"platform defaults: {safe_rel(root, p)}", p))

    if board_profile:
        b = board_profile.path.parent / "sdkconfig.defaults"
        if b.exists():
            parts.append((f"board defaults: {safe_rel(root, b)}", b))

    if not parts:
        parts = []

    out_lines: List[str] = []
    out_lines.append("# Auto-generated by tools/configure.py")
    out_lines.append("# Concatenated defaults below")
    out_lines.append("")

    if not parts:
        out_lines.append("# No platform/board sdkconfig defaults found.")
        out_lines.append("")

    for label, path in parts:
        block = path.read_text(encoding="utf-8", errors="replace").rstrip()
        if "CONFIG_SPIRAM=y" in block:
            spiram_enabled_by_defaults = True
        out_lines.append(f"# --- BEGIN {label} ---")
        out_lines.append(block)
        out_lines.append(f"# --- END {label} ---")
        out_lines.append("")

    # Driver-provided sdkconfig snippets (for enabled drivers only)
    mod_lines: List[str] = []
    for mid in enabled_modules:
        mod = modules.get(mid)
        if not mod:
            continue
        sdk_lines = mod.raw.get("sdkconfig") if mod.raw else None
        if isinstance(sdk_lines, list) and sdk_lines:
            mod_lines.append(f"# --- BEGIN driver defaults: {mid} ---")
            mod_lines.extend([str(x) for x in sdk_lines])
            mod_lines.append(f"# --- END driver defaults: {mid} ---")
            mod_lines.append("")

    # If PSRAM is not enabled via driver, explicitly disable it by default.
    if "psram" not in enabled_modules and not spiram_enabled_by_defaults:
        out_lines.append("# --- BEGIN driver defaults: psram (disabled) ---")
        out_lines.append("CONFIG_SPIRAM=n")
        out_lines.append("# --- END driver defaults: psram (disabled) ---")
        out_lines.append("")

    if mod_lines:
        out_lines.extend(mod_lines)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(out_lines).rstrip() + "\n", encoding="utf-8")


def append_driver_config_macros(out_path: Path, driver_config: Optional[dict]) -> None:
    if not driver_config or not isinstance(driver_config, dict):
        return
    def _skip_cfg_key(key: str) -> bool:
        k = str(key).strip().lower()
        return k.endswith("_name") or k.endswith("_content_b64")

    lines: List[str] = []
    lines.append("")
    lines.append("/* Driver configuration options */")
    for module_id, options in driver_config.items():
        if not isinstance(options, dict):
            continue
        for key, val in options.items():
            if _skip_cfg_key(str(key)):
                continue
            macro = f"BASALT_CFG_{slug_to_macro(str(module_id))}_{slug_to_macro(str(key))}"
            if isinstance(val, bool):
                lines.append(f"#define {macro} {1 if val else 0}")
            elif isinstance(val, int):
                lines.append(f"#define {macro} {val}")
            elif isinstance(val, float) and int(val) == val:
                lines.append(f"#define {macro} {int(val)}")
            elif isinstance(val, float):
                lines.append(f"#define {macro} {val}")
            else:
                lines.append(f"#define {macro} \"{val}\"")
    if lines:
        with out_path.open("a", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")




def _driver_option_allows_value(opt: dict, value) -> bool:
    opt_type = str(opt.get("type") or "").strip().lower()
    if opt_type == "select":
        options = opt.get("options") if isinstance(opt.get("options"), list) else []
        # Accept either exact match or string-equal for numeric/string select mixes.
        return any(value == o or str(value) == str(o) for o in options)

    if opt_type == "number":
        if not isinstance(value, (int, float)):
            return False
        min_v = opt.get("min")
        max_v = opt.get("max")
        if isinstance(min_v, (int, float)) and value < min_v:
            return False
        if isinstance(max_v, (int, float)) and value > max_v:
            return False
        return True

    if opt_type == "boolean":
        return isinstance(value, bool)

    if opt_type in {"string", "text"}:
        return isinstance(value, str)

    # Unknown option type: do not hard-fail here.
    return True


def validate_driver_config(
    modules: Dict[str, Module],
    enabled_modules: List[str],
    driver_config: Optional[dict],
) -> List[str]:
    errors: List[str] = []
    if not driver_config or not isinstance(driver_config, dict):
        return errors

    enabled = set(enabled_modules)

    for module_id, options in driver_config.items():
        if module_id not in modules:
            errors.append(f"driver_config references unknown module '{module_id}'")
            continue
        if module_id not in enabled:
            errors.append(f"driver_config provided for module '{module_id}' that is not enabled")
            continue
        if not isinstance(options, dict):
            errors.append(f"driver_config for module '{module_id}' must be an object")
            continue

        mod = modules[module_id]
        raw_opts = mod.raw.get("configuration_options") if isinstance(mod.raw, dict) else None
        opt_map = {}
        if isinstance(raw_opts, list):
            for entry in raw_opts:
                if isinstance(entry, dict) and isinstance(entry.get("id"), str):
                    opt_map[norm_slug(entry["id"])] = entry

        for key, val in options.items():
            k = norm_slug(str(key))
            if k.endswith("_name") or k.endswith("_content_b64"):
                continue
            if not opt_map:
                # Module has no explicit option schema.
                continue
            if k not in opt_map:
                errors.append(f"driver_config.{module_id}.{key}: unknown option")
                continue
            if not _driver_option_allows_value(opt_map[k], val):
                errors.append(f"driver_config.{module_id}.{key}: invalid value '{val}'")

    return errors


def validate_required_module_pins(
    modules: Dict[str, Module],
    enabled_modules: List[str],
    board_data: Optional[dict],
) -> List[str]:
    errors: List[str] = []
    if not board_data or not isinstance(board_data, dict):
        return errors

    pins = board_data.get("pins") if isinstance(board_data.get("pins"), dict) else {}

    for mid in enabled_modules:
        mod = modules.get(mid)
        if not mod or not isinstance(mod.raw, dict):
            continue
        reqs = mod.raw.get("pin_requirements")
        if not isinstance(reqs, dict):
            continue
        for pin_name, meta in reqs.items():
            if not isinstance(meta, dict):
                continue
            if not bool(meta.get("required", False)):
                continue
            if pin_name not in pins:
                errors.append(f"module '{mid}' requires pin '{pin_name}' but board profile does not define it")
                continue
            value = pins.get(pin_name)
            # Boards may encode pins as numeric GPIOs (e.g. ESP32) or symbolic
            # names (e.g. STM32 "PA2", AVR "D1", PIC "RC6").
            is_valid = False
            if isinstance(value, int):
                is_valid = value >= 0
            elif isinstance(value, str):
                is_valid = len(value.strip()) > 0
            if not is_valid:
                errors.append(f"module '{mid}' requires pin '{pin_name}' to be bound (got {value})")
    return errors


def append_applet_macros(out_path: Path, applets: List[str]) -> None:
    selected = [norm_slug(a) for a in applets if norm_slug(a)]
    csv_val = ",".join(selected)
    lines: List[str] = []
    lines.append("")
    lines.append("/* Applet selection */")
    lines.append(f"#define BASALT_APPLET_COUNT {len(selected)}")
    lines.append(f"#define BASALT_APPLETS_CSV \"{csv_val}\"")
    for aid in selected:
        lines.append(f"#define BASALT_APPLET_ENABLED_{slug_to_macro(aid)} 1")
    with out_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def append_market_app_macros(out_path: Path, market_apps: List[str]) -> None:
    selected = [norm_slug(a) for a in market_apps if norm_slug(a)]
    csv_val = ",".join(selected)
    lines: List[str] = []
    lines.append("")
    lines.append("/* Market app selection */")
    lines.append(f"#define BASALT_MARKET_APP_COUNT {len(selected)}")
    lines.append(f"#define BASALT_MARKET_APPS_CSV \"{csv_val}\"")
    for aid in selected:
        lines.append(f"#define BASALT_MARKET_APP_ENABLED_{slug_to_macro(aid)} 1")
    with out_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def write_applets_json(
    out_path: Path,
    platform: Optional[str],
    board_id: Optional[str],
    applets: List[str],
) -> None:
    selected = [norm_slug(a) for a in applets if norm_slug(a)]
    payload = {
        "generated_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "platform": platform or "",
        "board_id": board_id or "",
        "applets": selected,
    }
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


# -----------------------------
# Wizard: driver selection
# -----------------------------

def normalize_driver_aliases(items: List[str]) -> List[str]:
    aliases = {
        "shell_lite": "shell_min",
        "shell_minimal": "shell_min",
        "shell_spiritus": "shell_tiny",
        "shell_mini": "shell_tiny",
        "fs": "fs_spiffs",
    }
    out: List[str] = []
    for item in items:
        slug = norm_slug(item)
        out.append(aliases.get(slug, slug))
    return out


def default_drivers_for_board(board_profile: Optional[BoardProfile]) -> Set[str]:
    if not board_profile:
        return set()
    defaults = (((board_profile.data or {}).get("defaults") or {}).get("modules") or [])
    selected = {norm_slug(x) for x in defaults if norm_slug(x)}
    if selected:
        return selected

    caps = set(board_capabilities(board_profile.data) or [])
    target_id = str((board_profile.data or {}).get("target_profile", "")).lower()
    is_avr_like = board_profile.platform in {"avr", "atmega"} or target_id in {"mega2560", "atmega328p", "atmega32u4"}
    if "spi" in caps:
        selected.add("spi")
    if "uart" in caps:
        selected.add("uart")
    if "tft" in caps:
        selected.add("tft")
    selected.add("fs_spiffs")
    if "psram" in caps:
        selected.add("psram")
    if "uart" in caps:
        selected.add("shell_tiny" if is_avr_like else "shell_full")
    return selected


def wizard_select_modules(modules: Dict[str, Module], allowed: Optional[Set[str]], defaults: Optional[Set[str]] = None) -> Set[str]:
    candidates = sorted(modules.keys())
    if allowed is not None:
        candidates = [m for m in candidates if m in allowed]
    default_set = {d for d in (defaults or set()) if d in candidates}

    print("")
    print("Select drivers to enable")
    print("------------------------")
    print("Enter comma-separated ids (e.g., spi,uart,tft)")
    print("Press Enter to keep defaults shown with [x].")
    print("Dependencies will be added automatically.")
    print("Conflicts will be rejected.")
    print("")

    for mid in candidates:
        m = modules[mid]
        dep = f" deps=[{','.join(m.depends)}]" if m.depends else ""
        conf = f" conflicts=[{','.join(m.conflicts)}]" if m.conflicts else ""
        marker = "[x]" if mid in default_set else "[ ]"
        print(f"  {marker} {mid:12s} : {m.name}{dep}{conf}")
        if m.description.strip():
            print(f"      {m.description.strip()}")

    print("")
    default_csv = ",".join(sorted(default_set))
    choice = input(f"Enable drivers (comma separated) [{default_csv}]: ").strip()
    if not choice:
        return set(default_set)
    if norm_slug(choice) in {"none", "off"}:
        return set()
    selected = normalize_driver_aliases(norm_list_csv(choice))
    return set([x for x in selected if x])


def wizard_select_template(
    templates: Dict[str, ProjectTemplate],
    platform: Optional[str],
) -> Optional[ProjectTemplate]:
    allowed = [t for t in templates.values() if template_allowed_for_platform(t, platform)]
    if not allowed:
        return None
    lines: List[str] = []
    index: Dict[str, ProjectTemplate] = {}
    for t in sorted(allowed, key=lambda x: x.id):
        line = f"{t.id:20s} ({t.name})"
        lines.append(line)
        index[line] = t
    choice = prompt_choice("Select template", lines, allow_empty=True)
    if not choice:
        return None
    return index[choice]


def wizard_select_apps(title: str, available: List[str], defaults: List[str]) -> List[str]:
    if not available:
        return []
    print("")
    print(title)
    print("-" * len(title))
    print("Enter comma-separated ids.")
    print(f"Available: {', '.join(available)}")
    default_csv = ",".join(defaults)
    raw = input(f"Selected [{default_csv}]: ").strip()
    if not raw:
        return defaults
    selected = [norm_slug(x) for x in norm_list_csv(raw) if norm_slug(x)]
    return sorted(dict.fromkeys(selected))


def wizard_select_avr_settings(board_profile: Optional[BoardProfile]) -> Tuple[dict, dict]:
    board_defaults = ((board_profile.data or {}).get("defaults") or {}) if board_profile else {}
    default_clock = board_defaults.get("clock", {}) if isinstance(board_defaults.get("clock"), dict) else {}
    default_fuses = board_defaults.get("fuses", {}) if isinstance(board_defaults.get("fuses"), dict) else {}
    default_upload = board_defaults.get("upload", {}) if isinstance(board_defaults.get("upload"), dict) else {}
    clock_hz = prompt_text("AVR clock frequency (Hz)", str(default_clock.get("freq_hz", "")))
    upload: dict = {
        "method": prompt_text("Upload method", str(default_upload.get("method", "bootloader"))),
        "fqbn": prompt_text("FQBN", str(default_upload.get("fqbn", "arduino:avr:mega"))),
        "mcu": prompt_text("MCU", str(default_upload.get("mcu", "atmega2560"))),
        "programmer": prompt_text("Programmer", str(default_upload.get("programmer", "wiring"))),
        "port": prompt_text("Serial port (optional)", str(default_upload.get("port", ""))),
    }
    baud_raw = prompt_text("Upload baud", str(default_upload.get("baud", 115200)))
    fuses: dict = {
        "lfuse": prompt_text("LFUSE (optional)", str(default_fuses.get("lfuse", ""))),
        "hfuse": prompt_text("HFUSE (optional)", str(default_fuses.get("hfuse", ""))),
        "efuse": prompt_text("EFUSE (optional)", str(default_fuses.get("efuse", ""))),
        "lock": prompt_text("LOCK (optional)", str(default_fuses.get("lock", ""))),
    }
    clock: dict = {}
    if str(clock_hz).strip():
        clock["freq_hz"] = int(str(clock_hz).strip())
    if str(baud_raw).strip():
        upload["baud"] = int(str(baud_raw).strip())
    upload = {k: v for k, v in upload.items() if str(v).strip() != ""}
    fuses = {k: str(v).strip() for k, v in fuses.items() if str(v).strip()}
    return clock, {"upload": upload, "fuses": fuses}


def run_wizard(
    root: Path,
    modules: Dict[str, Module],
    templates: Dict[str, ProjectTemplate],
) -> Tuple[Optional[str], Optional[BoardProfile], Set[str], List[str], List[str], dict, dict, Optional[str], dict]:
    """
    Returns (platform, board_profile, requested_modules)
    """
    print("")
    print("BasaltOS Configure Wizard")
    print("=========================")

    # Match web configurator flow: board-first via taxonomy filters.
    # Platform is derived from selected board.
    boards = discover_boards(root, None)
    board_profile: Optional[BoardProfile] = wizard_select_board_hierarchy(boards)
    platform: Optional[str] = board_profile.platform if board_profile else None
    allow: Optional[Set[str]] = board_capabilities(board_profile.data) if board_profile else None

    template = None
    if prompt_yes_no("Apply a project template?", default=False):
        template = wizard_select_template(templates, platform)
        if template:
            print(f"[wizard] template selected: {template.id}")

    requested_defaults = default_drivers_for_board(board_profile)
    if template:
        requested_defaults.update({norm_slug(x) for x in template.enabled_drivers if norm_slug(x)})
    requested = wizard_select_modules(modules, allow, defaults=requested_defaults)

    available_applets = sorted(known_applet_ids(root))
    default_applets = [norm_slug(x) for x in (template.applets if template else ()) if norm_slug(x)]
    applets = wizard_select_apps("Select applets", available_applets, default_applets)

    available_market_apps = sorted(discover_market_app_ids(root, platform))
    default_market = [norm_slug(x) for x in (template.market_apps if template else ()) if norm_slug(x)]
    default_market = [x for x in default_market if x in available_market_apps]
    market_apps = wizard_select_apps("Select market apps", available_market_apps, default_market)

    avr_clock: dict = {}
    avr_config: dict = {"fuses": {}, "upload": {}}
    if platform in {"avr", "atmega"} and prompt_yes_no("Configure AVR clock/fuses/upload now?", default=False):
        try:
            avr_clock, avr_config = wizard_select_avr_settings(board_profile)
        except Exception as ex:
            print(f"[wizard] AVR settings ignored due to invalid value: {ex}")
            avr_clock, avr_config = {}, {"fuses": {}, "upload": {}}

    template_id = template.id if template else None
    driver_config = dict(template.driver_config) if template else {}
    return platform, board_profile, requested, applets, market_apps, avr_clock, avr_config, template_id, driver_config


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
    ap.add_argument("--enable", default=None, help="comma-separated driver ids to enable (legacy alias)")
    ap.add_argument("--enable-drivers", default=None, help="comma-separated driver ids to enable")
    ap.add_argument("--disable", default=None, help="comma-separated driver ids to disable (legacy alias)")
    ap.add_argument("--disable-drivers", default=None, help="comma-separated driver ids to disable (applied after enable/deps)")
    ap.add_argument("--wizard", action="store_true", help="interactive: choose platform, board, drivers, apps")
    ap.add_argument("--list-modules", action="store_true", help="list available drivers (legacy flag)")
    ap.add_argument("--list-drivers", action="store_true", help="list available drivers")
    ap.add_argument("--list-boards", action="store_true", help="list available boards (filtered by --platform if set)")
    ap.add_argument("--list-templates", action="store_true", help="list available project templates")
    ap.add_argument("--template", default=None, help="template id from config/templates/project_templates.json")
    ap.add_argument("--applets", default=None, help="comma-separated applet ids")
    ap.add_argument("--market-apps", default=None, help="comma-separated market app ids")
    ap.add_argument("--clock-hz", type=int, default=None, help="AVR: clock frequency in Hz")
    ap.add_argument("--fuse-l", default=None, help="AVR: LFUSE value (e.g. 0xFF)")
    ap.add_argument("--fuse-h", default=None, help="AVR: HFUSE value (e.g. 0xD8)")
    ap.add_argument("--fuse-e", default=None, help="AVR: EFUSE value (e.g. 0xFD)")
    ap.add_argument("--fuse-lock", default=None, help="AVR: LOCK fuse value")
    ap.add_argument("--upload-method", default=None, help="AVR: upload method (bootloader/updi/isp)")
    ap.add_argument("--upload-fqbn", default=None, help="AVR: arduino-cli FQBN")
    ap.add_argument("--upload-mcu", default=None, help="AVR: MCU id for avrdude")
    ap.add_argument("--upload-programmer", default=None, help="AVR: programmer id")
    ap.add_argument("--upload-baud", type=int, default=None, help="AVR: upload baud rate")
    ap.add_argument("--upload-port", default=None, help="AVR: serial/USB port")
    ap.add_argument("--outdir", default=str(gen_dir), help="output directory (default: config/generated)")
    args = ap.parse_args()

    try:
        modules = discover_modules(modules_dir)
    except Exception as ex:
        eprint(f"[configure] ERROR: {ex}")
        return 2

    boards = discover_boards(root, args.platform)
    templates = discover_templates(root)

    if args.list_modules or args.list_drivers:
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

    if args.list_templates:
        if not templates:
            print("(no templates found)")
            return 0
        for tid in sorted(templates.keys()):
            t = templates[tid]
            plats = ",".join(t.platforms) if t.platforms else "*"
            print(f"{t.id}\n  name: {t.name}\n  platforms: {plats}\n  drivers: {','.join(t.enabled_drivers)}\n  applets: {','.join(t.applets)}\n  market_apps: {','.join(t.market_apps)}\n")
        return 0

    board_profile: Optional[BoardProfile] = None
    allow: Optional[Set[str]] = None
    platform: Optional[str] = args.platform
    requested: Set[str] = set(norm_list_csv(args.enable_drivers or args.enable))
    disabled: Set[str] = set(norm_list_csv(args.disable_drivers or args.disable))
    selected_applets: List[str] = [norm_slug(x) for x in norm_list_csv(args.applets) if norm_slug(x)]
    selected_market_apps: List[str] = [norm_slug(x) for x in norm_list_csv(args.market_apps) if norm_slug(x)]
    selected_template: Optional[ProjectTemplate] = None
    driver_config: dict = {}
    avr_clock: dict = {}
    avr_fuses: dict = {}
    avr_upload: dict = {}

    if args.wizard:
        platform, board_profile, requested, selected_applets, selected_market_apps, avr_clock, avr_bundle, template_id, driver_config = run_wizard(root, modules, templates)
        allow = board_capabilities(board_profile.data) if board_profile else None
        avr_fuses = dict(avr_bundle.get("fuses", {}))
        avr_upload = dict(avr_bundle.get("upload", {}))
        if template_id and template_id in templates:
            selected_template = templates[template_id]
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
                return 3

        if not requested:
            requested = set()

        if args.template:
            tid = norm_slug(args.template)
            selected_template = templates.get(tid)
            if not selected_template:
                eprint(f"[configure] ERROR: unknown template '{args.template}'.")
                eprint("[configure] TIP: run: python tools/configure.py --list-templates")
                return 3
            if not template_allowed_for_platform(selected_template, platform):
                eprint(f"[configure] ERROR: template '{selected_template.id}' does not support platform '{platform}'.")
                return 3
            requested.update(selected_template.enabled_drivers)
            if not selected_applets:
                selected_applets = [norm_slug(x) for x in selected_template.applets if norm_slug(x)]
            if not selected_market_apps:
                selected_market_apps = [norm_slug(x) for x in selected_template.market_apps if norm_slug(x)]
            driver_config = dict(selected_template.driver_config)

        if args.clock_hz is not None:
            if args.clock_hz <= 0:
                eprint("[configure] ERROR: --clock-hz must be > 0")
                return 3
            avr_clock["freq_hz"] = int(args.clock_hz)
        for k, v in (("lfuse", args.fuse_l), ("hfuse", args.fuse_h), ("efuse", args.fuse_e), ("lock", args.fuse_lock)):
            if v is not None and str(v).strip():
                avr_fuses[k] = str(v).strip()
        for k, v in (
            ("method", args.upload_method),
            ("fqbn", args.upload_fqbn),
            ("mcu", args.upload_mcu),
            ("programmer", args.upload_programmer),
            ("port", args.upload_port),
        ):
            if v is not None and str(v).strip():
                avr_upload[k] = str(v).strip()
        if args.upload_baud is not None:
            avr_upload["baud"] = int(args.upload_baud)

    selected_applets = sorted(dict.fromkeys([norm_slug(x) for x in selected_applets if norm_slug(x)]))
    selected_market_apps = sorted(dict.fromkeys([norm_slug(x) for x in selected_market_apps if norm_slug(x)]))

    # Resolve drivers
    try:
        enabled_list = resolve_modules(modules, requested, allow=allow)
    except ConfigError as ex:
        eprint(f"[configure] ERROR: {ex}")
        if allow is not None:
            eprint("[configure] NOTE: board capabilities restriction is active.")
            eprint(f"[configure] Allowed drivers for board: {', '.join(sorted(allow))}")
        if board_profile:
            defaults_hint = sorted(default_drivers_for_board(board_profile))
            if defaults_hint:
                eprint(f"[configure] Board default drivers: {', '.join(defaults_hint)}")
                eprint(
                    "[configure] TIP: start from board defaults, then add drivers incrementally "
                    "to isolate compatibility issues."
                )
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

    cfg_errors = validate_driver_config(modules, enabled_list, driver_config)
    if cfg_errors:
        for msg in cfg_errors:
            eprint(f"[configure] ERROR: {msg}")
        return 7

    outdir = Path(args.outdir).resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    basalt_h = outdir / "basalt_config.h"
    features_j = outdir / "basalt.features.json"
    sdk_defaults = outdir / "sdkconfig.defaults"
    applets_j = outdir / "applets.json"
    config_j = outdir / "basalt_config.json"

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

    pin_errors = validate_required_module_pins(modules, enabled_list, board_data)
    if pin_errors:
        for msg in pin_errors:
            eprint(f"[configure] ERROR: {msg}")
        return 8

    available_market = discover_market_app_ids(root, platform)
    unsupported_market = [a for a in selected_market_apps if a not in available_market]
    if unsupported_market:
        eprint(f"[configure] ERROR: unsupported market app(s) for platform '{platform or '*'}': {', '.join(unsupported_market)}")
        eprint("[configure] TIP: use web configurator market page or edit config/market_apps.json")
        return 6
    known_applets = known_applet_ids(root)
    unknown_applets = [a for a in selected_applets if a not in known_applets]
    if unknown_applets:
        eprint(f"[configure] WARNING: unknown applet id(s): {', '.join(unknown_applets)}")
        eprint("           They will be emitted to config files but may not deploy at runtime.")

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
        enabled_modules=enabled_list,
        modules=modules,
    )
    append_driver_config_macros(basalt_h, driver_config)
    append_applet_macros(basalt_h, selected_applets)
    append_market_app_macros(basalt_h, selected_market_apps)
    write_applets_json(applets_j, platform, board_id, selected_applets)

    payload = {
        "generated_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "platform": platform or "",
        "board_id": board_id or "",
        "board_dir": board_dir or "",
        "target_profile": board_profile.target if board_profile else "",
        "enabled_drivers": enabled_list,
        "resolved_drivers": enabled_list,
        "driver_config": driver_config,
        "applets": selected_applets,
        "market_apps": selected_market_apps,
        "clock": avr_clock,
        "fuses": avr_fuses,
        "upload": avr_upload,
    }
    if selected_template:
        payload["template"] = selected_template.id
    # Legacy compatibility keys used in earlier releases.
    payload["enabled_modules"] = list(enabled_list)
    payload["resolved_modules"] = list(enabled_list)
    payload["module_config"] = dict(driver_config)
    config_j.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    print("")
    print("[configure] Generated:")
    print(f"  - {basalt_h.relative_to(root)}")
    print(f"  - {features_j.relative_to(root)}")
    print(f"  - {sdk_defaults.relative_to(root)}")
    print(f"  - {applets_j.relative_to(root)}")
    print(f"  - {config_j.relative_to(root)}")
    if board_profile:
        print(f"[configure] Board profile: {safe_rel(root, board_profile.path)}")

    print("")
    plat = (platform or "").lower()
    if plat == "esp32":
        print("[configure] Next steps (ESP-IDF):")
        print("  1) Ensure your build includes config/generated in include paths")
        print("  2) Apply defaults when building, e.g.:")
        print("     SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build")
    elif plat in {"avr", "atmega", "pic16"}:
        print("[configure] Next steps (AVR/PIC toolchain):")
        print("  1) Include generated headers from config/generated in your project build")
        print("  2) Build/flash using your selected upload method settings in basalt.features.json")
    else:
        print("[configure] Next steps:")
        print("  1) Include generated headers from config/generated in your project build")
        print("  2) Apply generated defaults from config/generated/sdkconfig.defaults if your toolchain supports it")
    print("")
    if enabled_list:
        print(f"[configure] Enabled drivers: {', '.join(enabled_list)}")
    else:
        print("[configure] Enabled drivers: (none)")
    if board_profile:
        defaults_hint = sorted(default_drivers_for_board(board_profile))
        if defaults_hint:
            print(f"[configure] Board default drivers: {', '.join(defaults_hint)}")
            if set(enabled_list) != set(defaults_hint):
                print("[configure] TIP: if troubleshooting, retry with board defaults first, then add extras.")
    print(f"[configure] Selected applets: {', '.join(selected_applets) if selected_applets else '(none)'}")
    print(f"[configure] Selected market apps: {', '.join(selected_market_apps) if selected_market_apps else '(none)'}")
    if avr_clock or avr_fuses or avr_upload:
        print(f"[configure] AVR overrides: clock={avr_clock or {}}, fuses={avr_fuses or {}}, upload={avr_upload or {}}")
    print("")

    # Warn if sdkconfig has stale PSRAM/flash settings for the selected board.
    if board_profile:
        sdk_path = root / "sdkconfig"
        if sdk_path.exists():
            text = sdk_path.read_text(encoding="utf-8", errors="replace")
            has_psram_cfg = "CONFIG_SPIRAM=y" in text
            caps = board_profile.data.get("capabilities", []) if isinstance(board_profile.data, dict) else []
            caps_set = {str(c).strip() for c in caps if str(c).strip()}
            if has_psram_cfg and "psram" not in enabled_list:
                print("[configure] WARNING: existing sdkconfig enables PSRAM, but psram driver is not enabled.")
                print("             Delete ./sdkconfig or reconfigure to avoid PSRAM init crashes.")
            if has_psram_cfg and "psram" not in caps_set:
                print("[configure] WARNING: existing sdkconfig enables PSRAM, but selected board does not have PSRAM.")
                print("             Delete ./sdkconfig or reconfigure to avoid PSRAM init crashes.")

            m = re.search(r"CONFIG_ESPTOOLPY_FLASHSIZE_(\\d+)MB=y", text)
            cfg_flash = int(m.group(1)) if m else None
            board_flash = parse_flash_mb(str(board_profile.data.get("flash", "")))
            if cfg_flash and board_flash and cfg_flash != board_flash:
                print(f"[configure] WARNING: sdkconfig flash size is {cfg_flash}MB, but board declares {board_flash}MB.")
                print("             Delete ./sdkconfig or reconfigure to avoid flashing/boot issues.")
            print("")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
