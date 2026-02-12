#!/usr/bin/env python3
"""
BasaltOS Configuration GUI - Backend Server
Provides REST API for board/driver configuration and generates config files
"""

import json
import sys
import tempfile
import subprocess
import shutil
import io
import zipfile
import os
import re
import glob
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Any, Tuple
from flask import Flask, jsonify, request, send_from_directory, send_file
try:
    from flask_cors import CORS
except ImportError:  # pragma: no cover - optional dependency fallback
    CORS = None

app = Flask(__name__)
if CORS:
    CORS(app)

# Configuration paths (adjust based on your BasaltOS structure)
TOOLS_DIR = Path(__file__).resolve().parent
BASALTOS_ROOT = TOOLS_DIR.parent
BOARDS_DIR = BASALTOS_ROOT / "boards"
MODULES_DIR = BASALTOS_ROOT / "modules"
PLATFORMS_DIR = BASALTOS_ROOT / "platforms"
TARGETS_DIR = BASALTOS_ROOT / "targets"
CONFIG_OUTPUT_DIR = BASALTOS_ROOT / "config" / "generated"
BUILD_DIR = BASALTOS_ROOT / "build"
SPIFFS_DIR = BASALTOS_ROOT / "spiffs"
SPIFFS_APPS_DIR = SPIFFS_DIR / "apps"
MARKET_APPS_CATALOG = BASALTOS_ROOT / "config" / "market_apps.json"

MANAGED_APPLETS_STATE_FILE = CONFIG_OUTPUT_DIR / ".spiffs_state" / "applets.json"
MANAGED_MARKET_APPS_STATE_FILE = CONFIG_OUTPUT_DIR / ".spiffs_state" / "market_apps.json"

# Local configurator scope guard: marketplace features are disabled by default.
MARKET_FEATURES_ENABLED = os.getenv("BASALTOS_LOCAL_MARKET_ENABLED", "0") == "1"


# Reuse core configurator logic for correctness
sys.path.append(str(BASALTOS_ROOT / "tools"))
import configure as cfg
import export_local_sync_payload as sync_export
from pic_codegen import generate_pic16_project, render_basalt_config_preview as render_pic16_preview
from avr_codegen import generate_avr_project, render_basalt_config_preview as render_avr_preview
from app_validation import validate_app_dir


class ConfigGenerator:
    """Generate BasaltOS configuration files"""
    
    def __init__(self):
        self.boards = {}
        self.boards_by_id = {}
        self.modules = {}
        self.platforms = {}
        self.targets = {}
        self.load_all_configs()
    
    def load_all_configs(self):
        """Load all board, driver, and platform configurations"""
        self.load_boards()
        self.load_modules()
        self.load_platforms()
        self.load_targets()

    @staticmethod
    def _utc_iso_now() -> str:
        return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    @staticmethod
    def _safe_slug(value: str) -> str:
        value = "".join(ch for ch in str(value).strip().lower() if ch.isalnum() or ch in ("_", "-"))
        return value.replace("-", "_")

    @staticmethod
    def _read_json(path: Path, default: Any) -> Any:
        try:
            if path.exists():
                return json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            pass
        return default

    @staticmethod
    def _write_json(path: Path, payload: Any) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    def _managed_ids_from_state(self, path: Path) -> set[str]:
        state = self._read_json(path, {})
        ids = state.get("managed_ids") if isinstance(state, dict) else []
        if not isinstance(ids, list):
            return set()
        return {self._safe_slug(x) for x in ids if self._safe_slug(x)}

    def _write_managed_state(self, path: Path, managed_ids: List[str]) -> None:
        payload = {
            "generated_at": self._utc_iso_now(),
            "managed_ids": sorted({self._safe_slug(x) for x in managed_ids if self._safe_slug(x)}),
        }
        self._write_json(path, payload)

    @staticmethod
    def _enabled_drivers_from_config(config_data: dict) -> List[str]:
        raw = config_data.get("enabled_drivers", config_data.get("enabled_modules", [])) or []
        if not isinstance(raw, list):
            return []
        return [str(x).strip() for x in raw if str(x).strip()]

    @staticmethod
    def _driver_config_from_config(config_data: dict) -> dict:
        raw = config_data.get("driver_config", config_data.get("module_config", {})) or {}
        return raw if isinstance(raw, dict) else {}

    @staticmethod
    def _avr_clock_from_config(config_data: dict, board: dict | None) -> dict | None:
        defaults = (board or {}).get("defaults") or {}
        clock = dict(defaults.get("clock") or {})
        incoming = config_data.get("clock")
        if isinstance(incoming, dict):
            clock.update(incoming)
        freq = clock.get("freq_hz")
        if freq is None:
            return clock if clock else None
        try:
            clock["freq_hz"] = int(freq)
        except Exception:
            raise ValueError("clock.freq_hz must be an integer.")
        if clock["freq_hz"] <= 0:
            raise ValueError("clock.freq_hz must be > 0.")
        return clock

    @staticmethod
    def _avr_fuses_from_config(config_data: dict, board: dict | None) -> dict | None:
        defaults = (board or {}).get("defaults") or {}
        merged = dict(defaults.get("fuses") or {})
        incoming = config_data.get("fuses")
        if isinstance(incoming, dict):
            merged.update(incoming)
        out: dict = {}
        for key in ("lfuse", "hfuse", "efuse", "lock"):
            val = merged.get(key)
            if val is None:
                continue
            sval = str(val).strip()
            if not sval:
                continue
            out[key] = sval
        return out if out else None

    @staticmethod
    def _avr_upload_from_config(config_data: dict, board: dict | None) -> dict | None:
        defaults = (board or {}).get("defaults") or {}
        merged = dict(defaults.get("upload") or {})
        incoming = config_data.get("upload")
        if isinstance(incoming, dict):
            merged.update(incoming)
        if not merged:
            return None
        if "baud" in merged and merged.get("baud") not in (None, ""):
            try:
                merged["baud"] = int(merged.get("baud"))
            except Exception:
                raise ValueError("upload.baud must be an integer.")
        for key in ("method", "fqbn", "mcu", "programmer", "port"):
            if key in merged and merged.get(key) is not None:
                merged[key] = str(merged.get(key)).strip()
        return merged

    @staticmethod
    def _with_legacy_driver_keys(payload: dict) -> dict:
        out = dict(payload)
        if "enabled_drivers" in out and "enabled_modules" not in out:
            out["enabled_modules"] = list(out["enabled_drivers"])
        if "resolved_drivers" in out and "resolved_modules" not in out:
            out["resolved_modules"] = list(out["resolved_drivers"])
        if "driver_config" in out and "module_config" not in out:
            out["module_config"] = dict(out["driver_config"])
        return out

    def _render_builtin_applet_files(self, applet_id: str) -> Dict[str, str] | None:
        aid = self._safe_slug(applet_id)
        if not aid:
            return None

        scripts: Dict[str, str] = {
            "blink": """print("[blink] start")
try:
    import basalt
except Exception as e:
    print("blink: basalt module unavailable:", e)
    basalt = None
if basalt and hasattr(basalt, "led"):
    for _ in range(8):
        basalt.led.set(1, 1, 1)
        basalt.timer.sleep_ms(150)
        basalt.led.off()
        basalt.timer.sleep_ms(150)
print("[blink] done")
""",
            "system_info": """print("[system_info] start")
gc_mod = None
for mod_name in ("gc",):
    try:
        gc_mod = __import__(mod_name)
        break
    except Exception:
        pass
if gc_mod and hasattr(gc_mod, "mem_free"):
    try:
        print("heap.free:", gc_mod.mem_free())
    except Exception as e:
        print("heap.free: unavailable:", e)
else:
    print("heap.free: unavailable (gc module not present)")
try:
    import basalt
    print("basalt.led:", "yes" if hasattr(basalt, "led") else "no")
    print("basalt.timer:", "yes" if hasattr(basalt, "timer") else "no")
except Exception as e:
    print("basalt import error:", e)
print("[system_info] done")
""",
            "tft_hello": """print("[tft_hello] start")
msg = "Hello from BasaltOS applet"
print(msg)
try:
    import basalt
    if hasattr(basalt, "ui") and hasattr(basalt.ui, "text"):
        basalt.ui.text(msg)
        print("tft_hello: rendered with basalt.ui.text")
except Exception as e:
    print("tft_hello: ui path unavailable:", e)
print("[tft_hello] done")
""",
            "sd_probe": """print("[sd_probe] start")
os_mod = None
try:
    import uos as os_mod
except:
    try:
        import os as os_mod
    except:
        os_mod = None

if os_mod is None:
    print("sd_probe: os/uos module unavailable in this MicroPython build")
else:
    try:
        entries = os_mod.listdir("/sd")
        print("sd.mounted: yes")
        print("sd.entries:", len(entries))
        i = 0
        for name in entries:
            print(" -", name)
            i = i + 1
            if i >= 20:
                break
    except:
        print("sd.mounted: no")
        print("sd_probe: listdir('/sd') failed")
print("[sd_probe] done")
""",
            "fs_smoke": """print("[fs_smoke] start")
import os
path = "/data/fs_smoke.txt"
try:
    with open(path, "w") as f:
        f.write("basalt fs smoke\\n")
    with open(path, "r") as f:
        print("readback:", f.read().strip())
    os.remove(path)
    print("fs_smoke: pass")
except Exception as e:
    print("fs_smoke: fail:", e)
print("[fs_smoke] done")
""",
            "i2c_scan": """print("[i2c_scan] start")
try:
    from machine import I2C, Pin
except Exception as e:
    print("i2c_scan: machine.I2C unavailable in this runtime:", e)
    print("i2c_scan: requires MicroPython machine module support")
else:
    try:
        i2c = I2C(0, scl=Pin(22), sda=Pin(21))
        found = i2c.scan()
        print("i2c.found:", [hex(x) for x in found])
    except Exception as e:
        print("i2c_scan bus error:", e)
print("[i2c_scan] done")
""",
            "wifi_status": """print("[wifi_status] start")
try:
    import network
    sta = network.WLAN(network.STA_IF)
    print("wifi.active:", sta.active())
    print("wifi.connected:", sta.isconnected())
    if sta.isconnected():
        print("wifi.ifconfig:", sta.ifconfig())
except Exception as e:
    print("wifi_status error:", e)
print("[wifi_status] done")
""",
            "remote_node": """print("[remote_node] start")
print("remote_node: placeholder applet script")
print("[remote_node] done")
""",
            "uart_echo": """print("[uart_echo] start")
print("uart_echo: placeholder applet script")
print("[uart_echo] done")
""",
        }

        main_py = scripts.get(aid)
        if not main_py:
            return None
        app_toml = (
            f'name = "{aid}"\n'
            'version = "0.1.0"\n'
            'entry = "main.py"\n'
            'kind = "applet"\n'
            'managed_by = "basalt_configurator"\n'
            'source = "builtin"\n'
        )
        return {"main.py": main_py, "app.toml": app_toml, ".ba": "builtin_applet\n"}

    def _sync_selected_applets_to_spiffs(self, applets: List[str]) -> Dict[str, Any]:
        SPIFFS_APPS_DIR.mkdir(parents=True, exist_ok=True)
        selected = [self._safe_slug(x) for x in (applets or [])]
        selected = [x for x in selected if x]

        managed_prev = self._managed_ids_from_state(MANAGED_APPLETS_STATE_FILE)
        deployed: List[str] = []
        removed: List[str] = []
        skipped_unknown: List[str] = []

        selected_set = set(selected)
        for aid in selected:
            files = self._render_builtin_applet_files(aid)
            if not files:
                skipped_unknown.append(aid)
                continue
            app_dir = SPIFFS_APPS_DIR / aid
            app_dir.mkdir(parents=True, exist_ok=True)
            for rel_path, content in files.items():
                out = app_dir / rel_path
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(content, encoding="utf-8")
            deployed.append(aid)

        to_remove = managed_prev.difference(selected_set)
        for aid in sorted(to_remove):
            app_dir = SPIFFS_APPS_DIR / aid
            marker = app_dir / ".ba"
            if app_dir.exists() and marker.exists():
                shutil.rmtree(app_dir, ignore_errors=True)
                removed.append(aid)

        self._write_managed_state(MANAGED_APPLETS_STATE_FILE, deployed)
        return {
            "deployed": sorted(set(deployed)),
            "removed": removed,
            "skipped_unknown": sorted(set(skipped_unknown)),
            "state_file": str(MANAGED_APPLETS_STATE_FILE),
        }

    def get_market_catalog(self, platform: str | None = None) -> List[dict]:
        def _dir_size_bytes(path: Path) -> int:
            total = 0
            try:
                for p in path.rglob("*"):
                    if p.is_file():
                        total += p.stat().st_size
            except Exception:
                return 0
            return total

        raw = self._read_json(MARKET_APPS_CATALOG, [])
        out: List[dict] = []
        if not isinstance(raw, list):
            return out
        for item in raw:
            if not isinstance(item, dict):
                continue
            app_id = self._safe_slug(item.get("id", ""))
            if not app_id:
                continue
            rec = dict(item)
            rec["id"] = app_id
            plats = rec.get("platforms")
            if platform and isinstance(plats, list) and platform not in plats:
                continue
            source_rel = str(rec.get("source_dir") or "").strip()
            if source_rel:
                src = (BASALTOS_ROOT / source_rel).resolve()
                try:
                    src.relative_to(BASALTOS_ROOT.resolve())
                    if src.exists() and src.is_dir():
                        rec["source_size_bytes"] = _dir_size_bytes(src)
                except Exception:
                    pass
            out.append(rec)
        out.sort(key=lambda x: x.get("name", x.get("id", "")))
        return out

    def _write_market_catalog(self, items: List[dict]) -> None:
        MARKET_APPS_CATALOG.parent.mkdir(parents=True, exist_ok=True)
        MARKET_APPS_CATALOG.write_text(json.dumps(items, indent=2) + "\n", encoding="utf-8")

    def upsert_market_catalog_item(self, item: dict) -> dict:
        app_id = self._safe_slug(item.get("id", ""))
        if not app_id:
            raise ValueError("Invalid app id")

        cur = self._read_json(MARKET_APPS_CATALOG, [])
        if not isinstance(cur, list):
            cur = []

        rec = dict(item)
        rec["id"] = app_id
        rec["source_dir"] = str(rec.get("source_dir") or f"apps/{app_id}.app")
        plats = rec.get("platforms")
        if not isinstance(plats, list) or not plats:
            rec["platforms"] = ["esp32"]
        else:
            rec["platforms"] = [str(x).strip() for x in plats if str(x).strip()]

        by_id = {self._safe_slug((x or {}).get("id", "")): x for x in cur if isinstance(x, dict)}
        by_id[app_id] = rec
        out = [v for _, v in sorted(by_id.items(), key=lambda kv: str((kv[1] or {}).get("name", kv[0])).lower())]
        self._write_market_catalog(out)
        return rec

    def install_market_zip(self, app_id: str, zip_bytes: bytes) -> Path:
        app_id = self._safe_slug(app_id)
        if not app_id:
            raise ValueError("Invalid app id")
        if not zip_bytes:
            raise ValueError("Uploaded package is empty")

        app_root = BASALTOS_ROOT / "apps"
        app_root.mkdir(parents=True, exist_ok=True)
        dst = app_root / f"{app_id}.app"

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

            validate_app_dir(src, check_py_syntax=False)

            if dst.exists():
                shutil.rmtree(dst, ignore_errors=True)
            shutil.copytree(src, dst)

        return dst

    def make_market_app_zip_bytes(self, app_id: str, platform: str | None = None) -> bytes:
        app_id = self._safe_slug(app_id)
        if not app_id:
            raise ValueError("Invalid app id")
        items = self.get_market_catalog(platform=platform)
        item = next((x for x in items if self._safe_slug(x.get("id", "")) == app_id), None)
        if not item:
            raise ValueError("Market app not found")
        source_rel = str(item.get("source_dir") or "").strip()
        if not source_rel:
            raise ValueError("Market app has no source_dir")
        src = (BASALTOS_ROOT / source_rel).resolve()
        try:
            src.relative_to(BASALTOS_ROOT.resolve())
        except Exception:
            raise ValueError("Invalid market app source path")
        if not src.exists() or not src.is_dir():
            raise ValueError("Market app source directory not found")

        bio = io.BytesIO()
        with zipfile.ZipFile(bio, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            for p in sorted(src.rglob("*")):
                if p.is_dir():
                    continue
                arcname = str(p.relative_to(src))
                zf.write(p, arcname)
        bio.seek(0)
        return bio.read()

    def _sync_selected_market_apps_to_spiffs(self, selected_market_apps: List[str], platform: str | None) -> Dict[str, Any]:
        SPIFFS_APPS_DIR.mkdir(parents=True, exist_ok=True)
        selected = [self._safe_slug(x) for x in (selected_market_apps or []) if self._safe_slug(x)]
        selected_set = set(selected)

        catalog = self.get_market_catalog(platform=platform)
        by_id = {self._safe_slug(x.get("id", "")): x for x in catalog}
        managed_prev = self._managed_ids_from_state(MANAGED_MARKET_APPS_STATE_FILE)

        deployed: List[str] = []
        removed: List[str] = []
        skipped_unknown: List[str] = []
        skipped_missing_source: List[str] = []

        for app_id in selected:
            item = by_id.get(app_id)
            if not item:
                skipped_unknown.append(app_id)
                continue
            source_rel = str(item.get("source_dir") or "").strip()
            if not source_rel:
                skipped_missing_source.append(app_id)
                continue
            src = (BASALTOS_ROOT / source_rel).resolve()
            try:
                src.relative_to(BASALTOS_ROOT.resolve())
            except Exception:
                skipped_missing_source.append(app_id)
                continue
            if not src.exists() or not src.is_dir():
                skipped_missing_source.append(app_id)
                continue

            dst = SPIFFS_APPS_DIR / app_id
            if dst.exists():
                shutil.rmtree(dst, ignore_errors=True)
            shutil.copytree(src, dst)
            (dst / ".bm").write_text("market_app\n", encoding="utf-8")
            deployed.append(app_id)

        to_remove = managed_prev.difference(selected_set)
        for app_id in sorted(to_remove):
            dst = SPIFFS_APPS_DIR / app_id
            marker = dst / ".bm"
            if dst.exists() and marker.exists():
                shutil.rmtree(dst, ignore_errors=True)
                removed.append(app_id)

        self._write_managed_state(MANAGED_MARKET_APPS_STATE_FILE, deployed)
        return {
            "deployed": sorted(set(deployed)),
            "removed": removed,
            "skipped_unknown": sorted(set(skipped_unknown)),
            "skipped_missing_source": sorted(set(skipped_missing_source)),
            "state_file": str(MANAGED_MARKET_APPS_STATE_FILE),
        }
    
    def load_boards(self):
        """Load board configurations from boards/ directory (recursively).

        Supported layouts:
          - boards/<id>.json
          - boards/<platform>/<board_id>/board.json
          - boards/<platform>/<board_id>/*.json
        """
        if not BOARDS_DIR.exists():
            print(f"Warning: Boards directory not found: {BOARDS_DIR}")
            return

        def derive_ids(board_file: Path, board_data: dict):
            # board_id: prefer folder name for board.json, else file stem
            if board_file.name == "board.json":
                board_id = board_file.parent.name
            else:
                board_id = board_file.stem

            # platform: prefer explicit field, else path segment under boards/
            platform = board_data.get("platform")
            if not platform:
                try:
                    rel = board_file.relative_to(BOARDS_DIR)
                    platform = rel.parts[0] if len(rel.parts) >= 2 else None
                except Exception:
                    platform = None
            return board_id, platform

        candidates = []
        candidates.extend(sorted(BOARDS_DIR.glob("*.json")))
        candidates.extend(sorted(BOARDS_DIR.rglob("board.json")))

        for board_file in candidates:

            try:
                with open(board_file, "r", encoding="utf-8") as f:
                    board_data = json.load(f)

                board_id, platform = derive_ids(board_file, board_data)
                if platform:
                    board_data["platform"] = platform

                board_data.setdefault("id", board_data.get("id") or board_id)
                self.boards[board_id] = board_data
                self.boards_by_id[str(board_data["id"]).strip()] = board_data
            except Exception as e:
                print(f"Error loading board {board_file}: {e}")

    def reload_boards(self):
        self.boards = {}
        self.boards_by_id = {}
        self.load_boards()

    def load_modules(self):
        """Load driver configurations from modules/ directory (recursively).

        Supported layouts:
          - modules/<id>.json
          - modules/<module_id>/module.json
        """
        if not MODULES_DIR.exists():
            print(f"Warning: Drivers directory not found: {MODULES_DIR}")
            return

        candidates = []
        candidates.extend(sorted(MODULES_DIR.glob("*.json")))
        candidates.extend(sorted(MODULES_DIR.rglob("module.json")))

        for module_file in candidates:

            try:
                with open(module_file, "r", encoding="utf-8") as f:
                    module_data = json.load(f)

                module_id = module_file.parent.name if module_file.name == "module.json" else module_file.stem
                module_data.setdefault("id", module_data.get("id") or module_id)
                self.modules[module_id] = module_data
            except Exception as e:
                print(f"Error loading driver {module_file}: {e}")

    def resolve_board(self, board_ref: str | None) -> dict | None:
        if not board_ref:
            return None
        if board_ref in self.boards:
            return self.boards[board_ref]
        if board_ref in self.boards_by_id:
            return self.boards_by_id[board_ref]
        return None

    def load_platforms(self):
        """Load platform configurations from platforms/ directory.

        Expected:
          - platforms/<platform_id>.json
        """
        if not PLATFORMS_DIR.exists():
            print(f"Warning: Platforms directory not found: {PLATFORMS_DIR}")
            return

        for platform_file in PLATFORMS_DIR.glob("*.json"):
            try:
                with open(platform_file, "r", encoding="utf-8") as f:
                    platform_data = json.load(f)
                    platform_id = platform_data.get("id") or platform_file.stem
                    platform_data["id"] = platform_id
                    self.platforms[platform_id] = platform_data
            except Exception as e:
                print(f"Error loading platform {platform_file}: {e}")

    def load_targets(self):
        """Load target profile configurations from targets/ directory."""
        if not TARGETS_DIR.exists():
            return

        for target_file in TARGETS_DIR.glob("*.json"):
            try:
                with open(target_file, "r", encoding="utf-8") as f:
                    target_data = json.load(f)
                    target_id = target_data.get("id") or target_file.stem
                    target_data["id"] = target_id
                    self.targets[target_id] = target_data
            except Exception as e:
                print(f"Error loading target {target_file}: {e}")


    def get_boards_by_platform(self, platform_id: str):
        """Return list of boards matching the given platform_id."""
        out = []
        pid = str(platform_id or "").strip().lower()
        aliases = {
            "avr": {"avr", "atmega"},
            "atmega": {"avr", "atmega"},
        }
        accepted = aliases.get(pid, {pid} if pid else set())
        for board_id, data in self.boards.items():
            plat = str(data.get("platform", "")).strip().lower()
            if (accepted and plat in accepted) or (not accepted and plat == pid):
                # ensure id is present for the GUI
                item = dict(data)
                item.setdefault("id", board_id)
                # Present ATmega-family boards under AVR umbrella in the GUI.
                if pid == "avr" and plat == "atmega":
                    item["platform"] = "avr"
                out.append(item)
        # stable ordering
        out.sort(key=lambda x: x.get("name", x.get("id", "")))
        return out

    def create_custom_board(self, payload: dict) -> dict:
        board_id = self._safe_slug(payload.get("id", ""))
        platform = self._safe_slug(payload.get("platform", ""))
        if not board_id:
            raise ValueError("Board id is required")
        if not platform:
            raise ValueError("Platform is required")
        if platform not in self.platforms:
            raise ValueError(f"Unknown platform '{platform}'")

        name = str(payload.get("name", "")).strip()
        if not name:
            raise ValueError("Board name is required")

        capabilities = payload.get("capabilities") or []
        if not isinstance(capabilities, list):
            raise ValueError("Capabilities must be a list")
        capabilities = [self._safe_slug(x) for x in capabilities if self._safe_slug(x)]

        pins_in = payload.get("pins") or {}
        if not isinstance(pins_in, dict):
            raise ValueError("pins must be an object")
        pins: Dict[str, Any] = {}
        for k, v in pins_in.items():
            sig = self._safe_slug(k)
            if not sig:
                continue
            if isinstance(v, bool):
                raise ValueError(f"Pin value for '{sig}' cannot be boolean")
            if isinstance(v, (int, float)) and int(v) == v:
                pins[sig] = int(v)
            else:
                raw = str(v).strip()
                try:
                    pins[sig] = int(raw)
                except ValueError:
                    pins[sig] = raw

        pin_defs_in = payload.get("pin_definitions") or {}
        pin_definitions: Dict[str, Any] = {}
        if isinstance(pin_defs_in, dict):
            for k, v in pin_defs_in.items():
                sig = self._safe_slug(k)
                if not sig or not isinstance(v, dict):
                    continue
                alts = v.get("alternatives")
                if not isinstance(alts, list):
                    alts = [pins.get(sig)] if sig in pins else []
                pin_definitions[sig] = {
                    "description": str(v.get("description") or sig),
                    "alternatives": [a for a in alts if a is not None],
                    "type": str(v.get("type") or "bidirectional"),
                }

        default_modules = payload.get("default_drivers", payload.get("default_modules")) or []
        if not isinstance(default_modules, list):
            default_modules = []
        default_modules = [self._safe_slug(x) for x in default_modules if self._safe_slug(x)]
        if not default_modules:
            if "gpio" in self.modules:
                default_modules.append("gpio")
            if "uart" in capabilities and "uart" in self.modules:
                default_modules.append("uart")
                if "shell_full" in self.modules:
                    default_modules.append("shell_full")
            if "spi" in capabilities and "spi" in self.modules:
                default_modules.append("spi")
            if "i2c" in capabilities and "i2c" in self.modules:
                default_modules.append("i2c")
            if "tft" in capabilities and "tft" in self.modules:
                default_modules.append("tft")
            if "sd_card" in capabilities and "fs_sd" in self.modules:
                default_modules.append("fs_sd")
            if "fs_spiffs" in self.modules:
                default_modules.append("fs_spiffs")

        board_data = {
            "id": board_id,
            "name": name,
            "description": str(payload.get("description") or f"Custom {platform} board"),
            "platform": platform,
            "target": str(payload.get("target") or platform),
            "mcu": str(payload.get("mcu") or "Unknown"),
            "flash": str(payload.get("flash") or ""),
            "ram": str(payload.get("ram") or ""),
            "display": str(payload.get("display") or "None"),
            "capabilities": sorted(set(capabilities)),
            "pins": pins,
            "pin_definitions": pin_definitions,
            "defaults": {
                "drivers": sorted(set(default_modules)),
                "modules": sorted(set(default_modules)),
                "applets": [],
            },
            "notes": str(payload.get("notes") or "Generated by BasaltOS DIY Board Creator"),
        }
        target_profile = str(payload.get("target_profile") or "").strip()
        if target_profile:
            board_data["target_profile"] = target_profile

        board_path = BOARDS_DIR / platform / board_id / "board.json"
        board_path.parent.mkdir(parents=True, exist_ok=True)
        board_path.write_text(json.dumps(board_data, indent=2) + "\n", encoding="utf-8")
        self.reload_boards()
        return {"board_path": str(board_path), "board": board_data}


    def get_available_modules(self, platform_id: str | None = None):
        """Return list of drivers, optionally filtered by platform.

        If a driver declares supported platforms (e.g. 'platforms' or 'supported_platforms'),
        it will be filtered accordingly. If no such field exists, the driver is treated as
        platform-agnostic and included.
        """
        out = []
        for module_id, data in self.modules.items():
            item = dict(data)
            item.setdefault("id", module_id)

            if platform_id:
                plats = item.get("platforms") or item.get("supported_platforms")
                if plats and platform_id not in plats:
                    continue
            out.append(item)

        out.sort(key=lambda x: x.get("name", x.get("id", "")))
        return out

    def get_available_drivers(self, platform_id: str | None = None):
        """Canonical alias for get_available_modules()."""
        return self.get_available_modules(platform_id)


    def _resolve(self, config_data: dict) -> Tuple[str | None, cfg.BoardProfile | None, List[str], Dict[str, cfg.Module]]:
        platform = config_data.get("platform")
        board_id = config_data.get("board_id")
        enabled_modules = self._enabled_drivers_from_config(config_data)

        modules = cfg.discover_modules(BASALTOS_ROOT / "modules")
        boards = cfg.discover_boards(BASALTOS_ROOT, platform)

        board_profile = None
        allow = None
        if platform and board_id:
            try:
                board_profile, allow = cfg.select_board_noninteractive(boards, platform, board_id)
            except cfg.ConfigError as ex:
                # AVR/ATmega compatibility alias: allow selecting ATmega boards from AVR UI flow.
                if str(platform).strip().lower() in {"avr", "atmega"}:
                    alt = "atmega" if str(platform).strip().lower() == "avr" else "avr"
                    try:
                        alt_boards = cfg.discover_boards(BASALTOS_ROOT, alt)
                        board_profile, allow = cfg.select_board_noninteractive(alt_boards, alt, board_id)
                    except cfg.ConfigError:
                        raise ValueError(str(ex))
                else:
                    raise ValueError(str(ex))
            if allow is not None:
                # Always allow internal SPIFFS on boards with defined capabilities
                allow.add("fs_spiffs")
                board_caps = board_profile.data.get("capabilities") if board_profile else None
                if isinstance(board_caps, list) and "sd_card" in board_caps:
                    allow.add("fs_sd")

        enabled_list = cfg.resolve_modules(modules, set(enabled_modules), allow=allow)
        return platform, board_profile, enabled_list, modules


    def _select_target(self, config_data: dict) -> dict | None:
        target_id = config_data.get("target_profile")
        if not target_id:
            board = self.resolve_board(config_data.get("board_id"))
            target_id = board.get("target_profile") if board else None
        if not target_id:
            return None
        return self.targets.get(target_id)


    def _validate_module_policy(self, target: dict, enabled_modules: List[str]) -> None:
        policy = target.get("module_policy") if isinstance(target, dict) else None
        if not policy:
            return
        mode = policy.get("mode")
        if mode == "allowlist":
            allowed = set(policy.get("allowed") or [])
            disallowed = [m for m in enabled_modules if m not in allowed]
            if disallowed:
                raise ValueError(f"Driver '{disallowed[0]}' is not supported by selected board/capabilities.")
        elif mode == "denylist":
            denied = set(policy.get("denied") or [])
            disallowed = [m for m in enabled_modules if m in denied]
            if disallowed:
                raise ValueError(f"Driver '{disallowed[0]}' is not supported by selected board/capabilities.")


    def _merge_pins(self, board: dict | None, custom_pins: dict | None) -> dict:
        pins = dict((board or {}).get("pins", {}) or {})
        if isinstance(custom_pins, dict):
            for k, v in custom_pins.items():
                if v in (-1, None, ""):
                    pins.pop(k, None)
                else:
                    pins[k] = v
        return pins

    def _signal_io_type(self, board: dict | None, signal: str) -> str:
        pin_defs = (board or {}).get("pin_definitions", {}) or {}
        pin_def = pin_defs.get(signal)
        if isinstance(pin_def, dict):
            t = str(pin_def.get("type", "")).strip().lower()
            if t in {"input", "output", "bidirectional"}:
                return t
        s = signal.lower()
        output_suffixes = (
            "_tx", "_mosi", "_sclk", "_clk", "_sck", "_cs", "_dc",
            "_rst", "_bl", "_led", "_pwm"
        )
        input_suffixes = ("_rx", "_miso", "_irq", "_int", "_button")
        if s.endswith(output_suffixes):
            return "output"
        if s.endswith(input_suffixes):
            return "input"
        return "bidirectional"

    def _spi_share_kind(self, signal: str) -> str | None:
        s = signal.lower()
        if s.endswith("_mosi"):
            return "mosi"
        if s.endswith("_miso"):
            return "miso"
        if s.endswith("_sclk") or s.endswith("_clk") or s.endswith("_sck"):
            return "sclk"
        return None

    def _validate_pins(self, board: dict | None, pins: dict) -> None:
        if not isinstance(pins, dict):
            raise ValueError("Pin map is invalid.")

        platform = str((board or {}).get("platform", "")).strip().lower()
        board_default_pins = ((board or {}).get("pins", {}) or {}) if isinstance((board or {}).get("pins", {}), dict) else {}
        pin_usage: Dict[Any, List[str]] = {}
        errors: List[str] = []

        for signal, raw in pins.items():
            sval = str(signal).strip()
            if not sval:
                continue

            value = raw
            if isinstance(value, str):
                txt = value.strip()
                if txt == "":
                    continue
                if txt.lstrip("-").isdigit():
                    value = int(txt, 10)
                else:
                    value = txt
            if value in (-1, None):
                continue

            if isinstance(value, int):
                if platform == "esp32":
                    if value < 0 or value > 39:
                        errors.append(f"{sval}: GPIO{value} is out of range for ESP32 (0-39).")
                        continue
                    if 6 <= value <= 11 and board_default_pins.get(sval) != value:
                        errors.append(f"{sval}: GPIO{value} is reserved for SPI flash and cannot be used.")
                    io_type = self._signal_io_type(board, sval)
                    if io_type in {"output", "bidirectional"} and 34 <= value <= 39:
                        errors.append(f"{sval}: GPIO{value} is input-only on ESP32.")

            pin_usage.setdefault(value, []).append(sval)

        for pin, signals in pin_usage.items():
            uniq = []
            for s in signals:
                if s not in uniq:
                    uniq.append(s)
            if len(uniq) <= 1:
                continue

            kinds = [self._spi_share_kind(s) for s in uniq]
            if all(k is not None for k in kinds) and len(set(kinds)) == 1:
                # Allow shared SPI data lines (e.g. tft_mosi + sd_mosi).
                continue

            if all(str(s).lower().endswith("_cs") for s in uniq):
                # Allow board-level shared CS helper defaults (e.g. spi_cs + can_cs).
                continue

            if all(board_default_pins.get(s) == pin for s in uniq):
                # Trust board-declared default multiplexing; still validate user overrides.
                continue

            errors.append(f"GPIO{pin} is assigned to multiple signals: {', '.join(uniq)}.")

        if errors:
            raise ValueError("Pin validation failed: " + " ".join(errors))


    def _board_data_with_pins(self, board_profile: cfg.BoardProfile | None, custom_pins: dict | None) -> dict | None:
        if not board_profile:
            return None
        data = dict(board_profile.data)
        if isinstance(custom_pins, dict) and custom_pins:
            pins = dict(data.get("pins", {}) or {})
            for k, v in custom_pins.items():
                pins[k] = v
            data["pins"] = pins
        return data


    def _append_driver_config_macros(self, out_path: Path, driver_config: dict | None) -> None:
        if not driver_config or not isinstance(driver_config, dict):
            return

        lines: List[str] = []
        lines.append("")
        lines.append("/* Driver configuration options */")
        for module_id, options in driver_config.items():
            if not isinstance(options, dict):
                continue
            for key, val in options.items():
                macro = f"BASALT_CFG_{cfg.slug_to_macro(str(module_id))}_{cfg.slug_to_macro(str(key))}"
                if isinstance(val, bool):
                    lines.append(f"#define {macro} {1 if val else 0}")
                elif isinstance(val, (int, float)) and int(val) == val:
                    lines.append(f"#define {macro} {int(val)}")
                elif isinstance(val, (int, float)):
                    lines.append(f"#define {macro} {val}")
                else:
                    lines.append(f"#define {macro} \"{val}\"")

        out_path.write_text(out_path.read_text(encoding="utf-8") + "\n".join(lines) + "\n", encoding="utf-8")

    def _append_applet_macros(self, out_path: Path, applets: List[str] | None) -> None:
        selected = [str(a).strip() for a in (applets or []) if str(a).strip()]
        csv_val = ",".join(selected).replace("\\", "\\\\").replace('"', '\\"')
        lines: List[str] = []
        lines.append("")
        lines.append("/* Applet selection */")
        lines.append(f"#define BASALT_APPLET_COUNT {len(selected)}")
        lines.append(f"#define BASALT_APPLETS_CSV \"{csv_val}\"")
        for aid in selected:
            macro = f"BASALT_APPLET_ENABLED_{cfg.slug_to_macro(aid)}"
            lines.append(f"#define {macro} 1")
        out_path.write_text(out_path.read_text(encoding="utf-8") + "\n".join(lines) + "\n", encoding="utf-8")

    def _append_market_app_macros(self, out_path: Path, market_apps: List[str] | None) -> None:
        selected = [str(a).strip() for a in (market_apps or []) if str(a).strip()]
        csv_val = ",".join(selected).replace("\\", "\\\\").replace('"', '\\"')
        lines: List[str] = []
        lines.append("")
        lines.append("/* Market app selection */")
        lines.append(f"#define BASALT_MARKET_APP_COUNT {len(selected)}")
        lines.append(f"#define BASALT_MARKET_APPS_CSV \"{csv_val}\"")
        for aid in selected:
            macro = f"BASALT_MARKET_APP_ENABLED_{cfg.slug_to_macro(aid)}"
            lines.append(f"#define {macro} 1")
        out_path.write_text(out_path.read_text(encoding="utf-8") + "\n".join(lines) + "\n", encoding="utf-8")

    def _write_applets_json(
        self,
        out_path: Path,
        platform: str | None,
        board_id: str | None,
        applets: List[str] | None,
    ) -> None:
        selected = [str(a).strip() for a in (applets or []) if str(a).strip()]
        payload = {
            "platform": platform,
            "board_id": board_id,
            "applets": selected,
            "count": len(selected),
        }
        out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


    def generate_basalt_config_h(
        self,
        board_id: str | None,
        enabled_drivers: List[str],
        custom_pins: dict | None,
        platform: str | None,
        driver_config: dict | None = None,
        applets: List[str] | None = None,
        market_apps: List[str] | None = None,
    ) -> str:
        platform, board_profile, enabled_list, modules = self._resolve({
            "platform": platform,
            "board_id": board_id,
            "enabled_drivers": enabled_drivers,
        })

        board_define_name = board_profile.id if board_profile else board_id
        board_data = self._board_data_with_pins(board_profile, custom_pins)
        board = self.resolve_board(board_define_name if board_define_name else board_id)
        if isinstance(board_data, dict):
            self._validate_pins(board, board_data.get("pins", {}) or {})

        with tempfile.TemporaryDirectory() as td:
            out_path = Path(td) / "basalt_config.h"
            cfg.emit_basalt_config_h(
                out_path=out_path,
                enabled_modules=enabled_list,
                modules=modules,
                platform=platform,
                board_define_name=board_define_name,
                board_data=board_data,
            )
            self._append_driver_config_macros(out_path, driver_config)
            self._append_applet_macros(out_path, applets)
            self._append_market_app_macros(out_path, market_apps)
            return out_path.read_text(encoding="utf-8")


    def generate_sdkconfig_defaults(self, board_id: str | None, platform: str | None, enabled_drivers: List[str]) -> str:
        platform, board_profile, enabled_list, modules = self._resolve({
            "platform": platform,
            "board_id": board_id,
            "enabled_drivers": enabled_drivers,
        })

        with tempfile.TemporaryDirectory() as td:
            out_path = Path(td) / "sdkconfig.defaults"
            cfg.emit_sdkconfig_defaults(
                out_path=out_path,
                root=BASALTOS_ROOT,
                platform=platform,
                board_profile=board_profile,
                enabled_modules=enabled_list,
                modules=modules,
            )
            return out_path.read_text(encoding="utf-8")


    def save_configuration(self, config_data: dict) -> Dict[str, str]:
        target = self._select_target(config_data)
        backend = target.get("codegen", {}).get("backend") if target else None
        if backend == "basalt_codegen_pic16_xc8":
            board_id = config_data.get("board_id")
            board = self.resolve_board(board_id)
            if not board:
                raise ValueError("Board not found.")

            enabled_modules = self._enabled_drivers_from_config(config_data)
            self._validate_module_policy(target, enabled_modules)
            custom_pins = config_data.get("custom_pins") or {}
            module_config = self._driver_config_from_config(config_data)
            applets = config_data.get("applets")
            if not applets:
                applets = (board.get("defaults") or {}).get("applets", [])
            pins = self._merge_pins(board, custom_pins)
            self._validate_pins(board, pins)

            outdir = CONFIG_OUTPUT_DIR / "pic16" / board_id
            outdir.mkdir(parents=True, exist_ok=True)
            files = generate_pic16_project(
                out_dir=outdir,
                board_id=board_id,
                target_id=target.get("id", "pic16"),
                enabled_modules=enabled_modules,
                pins=pins,
                module_config=module_config,
                clock=(board.get("defaults") or {}).get("clock"),
                fuses=(board.get("defaults") or {}).get("fuses"),
                has_interrupts=bool((target.get("capabilities") or {}).get("has_interrupts", True)),
            )

            config_json = outdir / "basalt_config.json"
            applets_json = outdir / "applets.json"
            payload = dict(config_data)
            payload["enabled_drivers"] = enabled_modules
            payload["driver_config"] = module_config
            payload = self._with_legacy_driver_keys(payload)
            payload["resolved_modules"] = enabled_modules
            payload["resolved_drivers"] = enabled_modules
            payload["target_profile"] = target.get("id")
            payload["pins"] = pins
            payload["applets"] = applets
            config_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
            self._write_applets_json(applets_json, board.get("platform"), board_id, applets)
            files["basalt_config.json"] = str(config_json)
            files["applets.json"] = str(applets_json)

            return files

        if backend == "basalt_codegen_avr_arduino":
            board_id = config_data.get("board_id")
            board = self.resolve_board(board_id)
            if not board:
                raise ValueError("Board not found.")

            enabled_modules = self._enabled_drivers_from_config(config_data)
            self._validate_module_policy(target, enabled_modules)
            custom_pins = config_data.get("custom_pins") or {}
            module_config = self._driver_config_from_config(config_data)
            applets = config_data.get("applets")
            if not applets:
                applets = (board.get("defaults") or {}).get("applets", [])
            pins = self._merge_pins(board, custom_pins)
            self._validate_pins(board, pins)
            clock = self._avr_clock_from_config(config_data, board)
            fuses = self._avr_fuses_from_config(config_data, board)
            upload_profile = self._avr_upload_from_config(config_data, board)

            outdir = CONFIG_OUTPUT_DIR / "avr" / board_id
            outdir.mkdir(parents=True, exist_ok=True)
            files = generate_avr_project(
                out_dir=outdir,
                board_id=board_id,
                target_id=target.get("id", "avr"),
                enabled_modules=enabled_modules,
                pins=pins,
                module_config=module_config,
                clock=clock,
                fuses=fuses,
                applets=applets,
                upload_profile=upload_profile,
            )

            config_json = outdir / "basalt_config.json"
            applets_json = outdir / "applets.json"
            payload = dict(config_data)
            payload["enabled_drivers"] = enabled_modules
            payload["driver_config"] = module_config
            payload = self._with_legacy_driver_keys(payload)
            payload["resolved_modules"] = enabled_modules
            payload["resolved_drivers"] = enabled_modules
            payload["target_profile"] = target.get("id")
            payload["pins"] = pins
            payload["applets"] = applets
            payload["clock"] = clock or {}
            payload["fuses"] = fuses or {}
            payload["upload"] = upload_profile or {}
            config_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
            self._write_applets_json(applets_json, board.get("platform"), board_id, applets)
            files["basalt_config.json"] = str(config_json)
            files["applets.json"] = str(applets_json)

            return files

        selected_board_id = config_data.get("board_id")
        platform, board_profile, enabled_list, modules = self._resolve(config_data)
        custom_pins = config_data.get("custom_pins") or {}
        module_config = self._driver_config_from_config(config_data)
        applets = config_data.get("applets")
        market_apps = config_data.get("market_apps") or []
        if not applets and board_profile:
            applets = ((board_profile.data or {}).get("defaults") or {}).get("applets", [])
        applets = applets or []

        board_define_name = board_profile.id if board_profile else config_data.get("board_id")
        board_id = board_profile.id if board_profile else config_data.get("board_id")
        board_dir = board_profile.board_dir if board_profile else None
        board_data = self._board_data_with_pins(board_profile, custom_pins)
        board = self.resolve_board(board_id) if board_id else None
        if isinstance(board_data, dict):
            self._validate_pins(board, board_data.get("pins", {}) or {})

        outdir = CONFIG_OUTPUT_DIR
        outdir.mkdir(parents=True, exist_ok=True)

        basalt_h = outdir / "basalt_config.h"
        features_j = outdir / "basalt.features.json"
        sdk_defaults = outdir / "sdkconfig.defaults"
        config_json = outdir / "basalt_config.json"
        applets_json = outdir / "applets.json"

        cfg.emit_basalt_config_h(
            out_path=basalt_h,
            enabled_modules=enabled_list,
            modules=modules,
            platform=platform,
            board_define_name=board_define_name,
            board_data=board_data,
        )
        self._append_driver_config_macros(basalt_h, module_config)
        self._append_applet_macros(basalt_h, applets)
        self._append_market_app_macros(basalt_h, market_apps)
        cfg.emit_features_json(
            out_path=features_j,
            enabled_modules=enabled_list,
            modules=modules,
            platform=platform,
            board_id=board_id,
            board_dir=board_dir,
        )
        cfg.emit_sdkconfig_defaults(
            out_path=sdk_defaults,
            root=BASALTOS_ROOT,
            platform=platform,
            board_profile=board_profile,
            enabled_modules=enabled_list,
            modules=modules,
        )

        payload = dict(config_data)
        payload["enabled_drivers"] = enabled_list
        payload["driver_config"] = module_config
        payload = self._with_legacy_driver_keys(payload)
        payload["resolved_modules"] = enabled_list
        payload["resolved_drivers"] = enabled_list
        payload["applets"] = applets
        payload["market_apps"] = market_apps
        config_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        self._write_applets_json(applets_json, platform, selected_board_id, applets)

        spiffs_deploy_report = None
        if platform == "esp32":
            applet_sync = self._sync_selected_applets_to_spiffs(applets)
            market_sync = self._sync_selected_market_apps_to_spiffs(market_apps, platform)
            spiffs_deploy_report = outdir / "spiffs_deploy_report.json"
            self._write_json(spiffs_deploy_report, {
                "generated_at": self._utc_iso_now(),
                "board_id": selected_board_id,
                "platform": platform,
                "applets": applet_sync,
                "market_apps": market_sync,
            })

        result = {
            "basalt_config.h": str(basalt_h),
            "basalt.features.json": str(features_j),
            "sdkconfig.defaults": str(sdk_defaults),
            "basalt_config.json": str(config_json),
            "applets.json": str(applets_json),
        }
        if spiffs_deploy_report:
            result["spiffs_deploy_report.json"] = str(spiffs_deploy_report)
        return result


# Instantiate the config generator (loads boards/modules/platforms)
config_gen = ConfigGenerator()


ESP32_FLASH_ARTIFACTS = {
    "bootloader.bin": {"path": BUILD_DIR / "bootloader" / "bootloader.bin", "offset": 0x1000, "required": True},
    "partition-table.bin": {"path": BUILD_DIR / "partition_table" / "partition-table.bin", "offset": 0x8000, "required": True},
    "ota_data_initial.bin": {"path": BUILD_DIR / "ota_data_initial.bin", "offset": 0xD000, "required": True},
    "basaltos.bin": {"path": BUILD_DIR / "basaltos.bin", "offset": 0x10000, "required": True},
    "storage.bin": {"path": BUILD_DIR / "storage.bin", "offset": 0x200000, "required": False},
}


def _esp32_flash_status() -> dict:
    files = []
    missing_required = []
    newest_mtime_ns = 0
    for name, meta in ESP32_FLASH_ARTIFACTS.items():
        path = meta["path"]
        exists = path.exists() and path.is_file()
        size = path.stat().st_size if exists else 0
        mtime_ns = path.stat().st_mtime_ns if exists else 0
        if mtime_ns > newest_mtime_ns:
            newest_mtime_ns = mtime_ns
        files.append({
            "name": name,
            "offset": int(meta["offset"]),
            "required": bool(meta["required"]),
            "exists": exists,
            "size": size,
            "mtime_ns": mtime_ns,
            "path": str(path),
        })
        if meta["required"] and not exists:
            missing_required.append(name)

    return {
        "ready": len(missing_required) == 0,
        "missing_required": missing_required,
        "files": files,
        "build_dir": str(BUILD_DIR),
        "build_version": str(newest_mtime_ns) if newest_mtime_ns else "",
    }


def _serial_ports_status() -> dict:
    def detect_serial_ports() -> List[str]:
        found: set[str] = set()
        try:
            from serial.tools import list_ports as pyserial_list_ports  # type: ignore
            for p in pyserial_list_ports.comports():
                dev = str(getattr(p, "device", "") or "").strip()
                if dev:
                    found.add(dev)
        except Exception:
            pass

        if os.name == "nt":
            for i in range(1, 257):
                found.add(f"COM{i}")
        else:
            for pat in ("/dev/ttyUSB*", "/dev/ttyACM*", "/dev/cu.usb*", "/dev/cu.SLAB*", "/dev/tty.usb*"):
                for p in glob.glob(pat):
                    found.add(p)
        return sorted(found)

    ports = detect_serial_ports()
    out = []
    for raw_path in ports:
        p = Path(raw_path)
        holder_lines: List[str] = []
        is_windows_com = bool(re.match(r"^COM\d+$", raw_path, flags=re.IGNORECASE))
        if not is_windows_com:
            try:
                proc = subprocess.run(
                    ["lsof", str(p)],
                    capture_output=True,
                    text=True,
                    timeout=2,
                    check=False,
                )
                txt = (proc.stdout or "").strip()
                if txt:
                    lines = txt.splitlines()
                    holder_lines = lines[1:] if len(lines) > 1 else lines
            except Exception:
                holder_lines = []
        out.append({
            "path": raw_path,
            "busy": len(holder_lines) > 0,
            "holders": holder_lines[:8],
        })
    return {
        "ports": out,
        "any_busy": any(item["busy"] for item in out),
    }

@app.route('/api/platforms', methods=['GET'])
def get_platforms():
    """Get list of available platforms"""
    config_gen.load_all_configs()
    hidden_aliases = {"atmega"}
    platforms = []
    for platform_id, platform_data in config_gen.platforms.items():
        if platform_id in hidden_aliases:
            continue
        platforms.append({
            'id': platform_id,
            'name': platform_data.get('name', platform_id),
            'description': platform_data.get('description', ''),
            'toolchain': platform_data.get('toolchain', '')
        })
    return jsonify(platforms)


@app.route('/api/boards/<platform>', methods=['GET'])
def get_boards(platform):
    """Get boards for a specific platform"""
    config_gen.load_all_configs()
    boards = config_gen.get_boards_by_platform(platform)
    return jsonify(boards)


@app.route('/api/targets', methods=['GET'])
def get_targets():
    """Get target profiles"""
    config_gen.load_all_configs()
    return jsonify(list(config_gen.targets.values()))


@app.route('/api/drivers', methods=['GET'])
@app.route('/api/modules', methods=['GET'])
def get_modules():
    """Get available drivers (legacy alias: /api/modules)."""
    platform = request.args.get('platform')
    drivers = getattr(config_gen, 'get_available_drivers', lambda p: [dict(v, id=k) for k,v in config_gen.modules.items()])(platform)
    return jsonify(drivers)


@app.route('/api/market/catalog', methods=['GET'])
def get_market_catalog():
    """Get curated app catalog for preinstall packaging (disabled by default in local mode)."""
    if not MARKET_FEATURES_ENABLED:
        return jsonify({"success": False, "error": "Local market endpoints are disabled in local configurator mode."}), 404
    platform = request.args.get('platform')
    return jsonify(config_gen.get_market_catalog(platform))


@app.route('/api/market/download/<app_id>', methods=['GET'])
def market_download(app_id):
    """Download a market app package as zip (disabled by default in local mode)."""
    if not MARKET_FEATURES_ENABLED:
        return jsonify({"success": False, "error": "Local market endpoints are disabled in local configurator mode."}), 404
    platform = request.args.get('platform')
    try:
        data = config_gen.make_market_app_zip_bytes(app_id, platform=platform)
        resp = send_file(
            io.BytesIO(data),
            mimetype="application/zip",
            as_attachment=True,
            download_name=f"{config_gen._safe_slug(app_id)}.zip",
        )
        resp.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
        return resp
    except ValueError as e:
        return jsonify({"success": False, "error": str(e)}), 400
    except Exception as e:
        return jsonify({"success": False, "error": f"Download failed: {e}"}), 500


@app.route('/api/market/upload', methods=['POST'])
def market_upload():
    """Upload a market app zip and add/update it in local catalog (disabled by default in local mode)."""
    if not MARKET_FEATURES_ENABLED:
        return jsonify({"success": False, "error": "Local market endpoints are disabled in local configurator mode."}), 404
    try:
        if "package" not in request.files:
            return jsonify({"success": False, "error": "Missing file field 'package'"}), 400
        pkg = request.files["package"]
        if not pkg or not pkg.filename:
            return jsonify({"success": False, "error": "No file selected"}), 400

        app_id = config_gen._safe_slug(request.form.get("id") or Path(pkg.filename).stem)
        if not app_id:
            return jsonify({"success": False, "error": "Invalid app id"}), 400

        name = str(request.form.get("name") or app_id).strip()
        description = str(request.form.get("description") or "").strip()
        version = str(request.form.get("version") or "0.1.0").strip()
        author = str(request.form.get("author") or "").strip()
        platforms_raw = str(request.form.get("platforms") or "esp32")
        plats = [p.strip().lower() for p in platforms_raw.replace(";", ",").split(",") if p.strip()]
        if not plats:
            plats = ["esp32"]

        raw_bytes = pkg.read()
        config_gen.install_market_zip(app_id, raw_bytes)
        rec = config_gen.upsert_market_catalog_item({
            "id": app_id,
            "name": name,
            "description": description,
            "version": version,
            "author": author,
            "platforms": plats,
            "source_dir": f"apps/{app_id}.app",
        })
        return jsonify({"success": True, "app": rec})
    except ValueError as e:
        return jsonify({"success": False, "error": str(e)}), 400
    except zipfile.BadZipFile:
        return jsonify({"success": False, "error": "Invalid ZIP package"}), 400
    except Exception as e:
        return jsonify({"success": False, "error": f"Upload failed: {e}"}), 500


@app.route('/api/board/<board_id>', methods=['GET'])
def get_board_details(board_id):
    """Get detailed information about a specific board"""
    board = config_gen.resolve_board(board_id)
    if not board:
        return jsonify({'error': 'Board not found'}), 404
    
    return jsonify({
        'id': board_id,
        'name': board.get('name', board_id),
        'description': board.get('description', ''),
        'mcu': board.get('mcu', 'Unknown'),
        'platform': board.get('platform', 'Unknown'),
        'flash': board.get('flash', 'Unknown'),
        'ram': board.get('ram', 'Unknown'),
        'display': board.get('display', 'None'),
        'target_profile': board.get('target_profile'),
        'pins': board.get('pins', {}),
        'capabilities': board.get('capabilities', [])
    })


@app.route('/api/boards/custom', methods=['POST'])
def create_custom_board():
    """Create/update a custom board JSON under boards/<platform>/<id>/board.json."""
    try:
        data = request.get_json(silent=True) or {}
        result = config_gen.create_custom_board(data)
        return jsonify({
            "success": True,
            "message": "Custom board saved.",
            "board_path": result["board_path"],
            "board": result["board"],
        })
    except ValueError as e:
        return jsonify({"success": False, "error": str(e)}), 400
    except Exception as e:
        return jsonify({"success": False, "error": f"Failed to create custom board: {e}"}), 500


@app.route('/api/templates', methods=['GET'])
def get_templates():
    """Get project templates"""
    templates_file = BASALTOS_ROOT / "config" / "templates" / "project_templates.json"
    if templates_file.exists():
        with open(templates_file, "r", encoding="utf-8") as f:
            return jsonify(json.load(f))
    return jsonify({"templates": []})


@app.route('/api/sync/export-preview', methods=['GET'])
def sync_export_preview():
    """Return sync export preview envelope summary for local workflow validation."""
    try:
        local_root = sync_export.resolve_local_data_root(BASALTOS_ROOT, None)
        envelope = sync_export.build_envelope(BASALTOS_ROOT, local_root, "v0.1.0")
        include_content = str(request.args.get("include_content", "")).strip().lower() in ("1", "true", "yes")

        type_counts: Dict[str, int] = {}
        for item in envelope.get("items", []):
            t = str(item.get("item_type", ""))
            type_counts[t] = type_counts.get(t, 0) + 1

        if not include_content:
            preview_items = []
            for item in envelope.get("items", []):
                preview_items.append({
                    "item_type": item.get("item_type"),
                    "item_id": item.get("item_id"),
                    "updated_utc": item.get("updated_utc"),
                    "content_hash": item.get("content_hash"),
                })
            envelope = {**envelope, "items": preview_items}

        return jsonify({
            "success": True,
            "schema_version": envelope.get("schema_version"),
            "item_count": len(envelope.get("items", [])),
            "type_counts": type_counts,
            "envelope": envelope,
        })
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500


@app.route('/api/flash/esp32/status', methods=['GET'])
def flash_esp32_status():
    """Return availability of flash artifacts for ESP32 web flasher."""
    resp = jsonify(_esp32_flash_status())
    resp.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
    resp.headers["Pragma"] = "no-cache"
    return resp


@app.route('/api/flash/esp32/ports', methods=['GET'])
def flash_esp32_ports():
    """Return host serial port lock/busy status for troubleshooting."""
    return jsonify(_serial_ports_status())


@app.route('/api/flash/esp32/manifest', methods=['GET'])
def flash_esp32_manifest():
    """Return an esp-web-tools manifest for flashing ESP32 from browser."""
    status = _esp32_flash_status()
    if not status["ready"]:
        return jsonify({
            "error": "ESP32 firmware artifacts are missing. Build first (idf.py -B build build).",
            "status": status,
        }), 404

    board_ref = request.args.get("board_id", "").strip()
    board = config_gen.resolve_board(board_ref) if board_ref else None
    board_name = (board or {}).get("name", "BasaltOS ESP32")

    parts = []
    for name, meta in ESP32_FLASH_ARTIFACTS.items():
        path = meta["path"]
        if not path.exists():
            continue
        version = str(path.stat().st_mtime_ns)
        parts.append({
            "path": f"/api/flash/esp32/files/{name}?v={version}",
            "offset": int(meta["offset"]),
        })

    manifest = {
        "name": f"{board_name} - BasaltOS",
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": parts,
            }
        ],
    }
    resp = jsonify(manifest)
    resp.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
    resp.headers["Pragma"] = "no-cache"
    return resp


@app.route('/api/flash/esp32/files/<artifact>', methods=['GET'])
def flash_esp32_file(artifact):
    """Serve a specific ESP32 firmware artifact file."""
    meta = ESP32_FLASH_ARTIFACTS.get(artifact)
    if not meta:
        return jsonify({"error": "Unknown artifact"}), 404
    path = meta["path"]
    if not path.exists() or not path.is_file():
        return jsonify({"error": f"Artifact not found: {artifact}"}), 404
    resp = send_from_directory(str(path.parent), path.name)
    resp.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
    resp.headers["Pragma"] = "no-cache"
    return resp


@app.route('/api/flash/esp32/local', methods=['POST'])
def flash_esp32_local():
    """Flash ESP32 locally via idf.py as a fallback when Web Serial is unreliable."""
    data = request.get_json(silent=True) or {}
    port = str(data.get("port", "")).strip()
    if not port:
        status = _serial_ports_status()
        ports = [p["path"] for p in status.get("ports", [])]
        if not ports:
            return jsonify({"success": False, "error": "No serial ports found (/dev/ttyUSB* or /dev/ttyACM*)."}), 404
        port = ports[0]

    if not Path(port).exists():
        return jsonify({"success": False, "error": f"Serial port not found: {port}"}), 404

    cmd = (
        "source tools/env.sh && "
        "SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults "
        f"idf.py -D SDKCONFIG=config/generated/sdkconfig -p {port} -B build flash"
    )
    try:
        proc = subprocess.run(
            ["bash", "-lc", cmd],
            cwd=str(BASALTOS_ROOT),
            capture_output=True,
            text=True,
            timeout=420,
            check=False,
        )
        combined = ((proc.stdout or "") + "\n" + (proc.stderr or "")).strip()
        tail = "\n".join(combined.splitlines()[-220:])
        if proc.returncode == 0:
            return jsonify({
                "success": True,
                "port": port,
                "returncode": proc.returncode,
                "output": tail,
            })
        return jsonify({
            "success": False,
            "port": port,
            "returncode": proc.returncode,
            "error": "Local flash failed.",
            "output": tail,
        }), 500
    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "port": port, "error": "Local flash timed out."}), 504


@app.route('/api/build/esp32', methods=['POST'])
def build_esp32():
    """Build ESP32 firmware using current generated configuration."""
    cmd = (
        "source tools/env.sh && "
        "SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults "
        "idf.py -D SDKCONFIG=config/generated/sdkconfig -B build build"
    )
    try:
        proc = subprocess.run(
            ["bash", "-lc", cmd],
            cwd=str(BASALTOS_ROOT),
            capture_output=True,
            text=True,
            timeout=420,
            check=False,
        )
        combined = ((proc.stdout or "") + "\n" + (proc.stderr or "")).strip()
        tail = "\n".join(combined.splitlines()[-240:])
        if proc.returncode == 0:
            return jsonify({
                "success": True,
                "returncode": proc.returncode,
                "output": tail,
            })
        return jsonify({
            "success": False,
            "returncode": proc.returncode,
            "error": "ESP32 build failed.",
            "output": tail,
        }), 500
    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "error": "ESP32 build timed out."}), 504


def _resolve_avr_project(board_id: str | None) -> tuple[Path | None, str | None]:
    bid = str(board_id or "").strip()
    if bid:
        proj = CONFIG_OUTPUT_DIR / "avr" / bid
        if proj.exists() and proj.is_dir():
            return proj, bid
        # When a board is explicitly requested, do not silently fall back to
        # another board's generated project.
        return None, bid
    root = CONFIG_OUTPUT_DIR / "avr"
    if not root.exists():
        return None, None
    candidates = sorted([p for p in root.iterdir() if p.is_dir()], key=lambda p: p.stat().st_mtime, reverse=True)
    if not candidates:
        return None, None
    return candidates[0], candidates[0].name


def _read_avr_meta(project_dir: Path) -> dict:
    meta_file = project_dir / "arduino.json"
    if not meta_file.exists():
        return {}
    try:
        return json.loads(meta_file.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _avr_flash_status(board_id: str | None = None) -> dict:
    project_dir, resolved_board = _resolve_avr_project(board_id)
    arduino_cli = shutil.which("arduino-cli")
    avrdude = shutil.which("avrdude")
    if not project_dir:
        return {
            "ready": False,
            "error": "No generated AVR project found. Run Generate first.",
            "board_id": resolved_board or str(board_id or ""),
            "tools": {
                "arduino_cli": bool(arduino_cli),
                "avrdude": bool(avrdude),
            },
        }
    meta = _read_avr_meta(project_dir)
    ino = list(project_dir.glob("*.ino"))
    return {
        "ready": len(ino) > 0,
        "board_id": resolved_board or str(board_id or ""),
        "project_dir": str(project_dir),
        "meta": meta,
        "tools": {
            "arduino_cli": bool(arduino_cli),
            "avrdude": bool(avrdude),
        },
    }


def _pick_default_serial_port() -> str:
    status = _serial_ports_status()
    ports = status.get("ports", []) or []
    free = [p["path"] for p in ports if not p.get("busy")]
    if free:
        return str(free[0])
    if ports:
        return str(ports[0].get("path", ""))
    return ""


def _run_cmd(args: List[str], cwd: Path, timeout_s: int = 240) -> tuple[int, str]:
    proc = subprocess.run(
        args,
        cwd=str(cwd),
        capture_output=True,
        text=True,
        timeout=timeout_s,
        check=False,
    )
    combined = ((proc.stdout or "") + "\n" + (proc.stderr or "")).strip()
    tail = "\n".join(combined.splitlines()[-240:])
    return proc.returncode, tail


def _extract_build_error_detail(output: str) -> str:
    lines = [ln.strip() for ln in str(output or "").splitlines() if ln.strip()]
    if not lines:
        return ""
    # Prefer concrete compiler/tool error lines.
    for ln in lines:
        low = ln.lower()
        if " error:" in low or low.startswith("error:") or "fatal error:" in low:
            return ln
        if "undefined reference" in low or "collect2: error" in low:
            return ln
    # Fall back to the last non-empty line if no explicit marker was found.
    return lines[-1]


@app.route('/api/flash/avr/status', methods=['GET'])
def flash_avr_status():
    board_id = request.args.get("board_id")
    return jsonify(_avr_flash_status(board_id))


@app.route('/api/flash/avr/ports', methods=['GET'])
def flash_avr_ports():
    return jsonify(_serial_ports_status())


@app.route('/api/build/avr', methods=['POST'])
def build_avr():
    data = request.get_json(silent=True) or {}
    project_dir, board_id = _resolve_avr_project(data.get("board_id"))
    if not project_dir:
        return jsonify({"success": False, "error": "No generated AVR project found. Run Generate first."}), 404
    if not shutil.which("arduino-cli"):
        return jsonify({"success": False, "error": "arduino-cli not found in PATH."}), 500

    meta = _read_avr_meta(project_dir)
    req_upload = data.get("upload") if isinstance(data.get("upload"), dict) else {}
    fqbn = str((req_upload.get("fqbn") if req_upload else None) or meta.get("board") or "arduino:avr:mega").strip()
    try:
        rc, out = _run_cmd(["arduino-cli", "compile", "--fqbn", fqbn, str(project_dir)], cwd=project_dir, timeout_s=420)
        if rc == 0:
            return jsonify({
                "success": True,
                "board_id": board_id,
                "project_dir": str(project_dir),
                "fqbn": fqbn,
                "returncode": rc,
                "output": out,
            })
        return jsonify({
            "success": False,
            "board_id": board_id,
            "fqbn": fqbn,
            "returncode": rc,
            "error": "AVR build failed.",
            "error_detail": _extract_build_error_detail(out),
            "output": out,
        }), 500
    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "error": "AVR build timed out."}), 504


@app.route('/api/flash/avr/local', methods=['POST'])
def flash_avr_local():
    data = request.get_json(silent=True) or {}
    project_dir, board_id = _resolve_avr_project(data.get("board_id"))
    if not project_dir:
        return jsonify({"success": False, "error": "No generated AVR project found. Run Generate first."}), 404

    meta = _read_avr_meta(project_dir)
    upload = dict(meta or {})
    req_upload = data.get("upload")
    if isinstance(req_upload, dict):
        upload.update(req_upload)

    method = str(upload.get("upload_method") or upload.get("method") or "bootloader").strip().lower()
    fqbn = str(upload.get("board") or upload.get("fqbn") or "arduino:avr:mega").strip()
    mcu = str(upload.get("cpu") or upload.get("mcu") or "atmega2560").strip()
    programmer = str(upload.get("programmer") or ("serialupdi" if method == "updi" else "wiring")).strip()
    baud = upload.get("baud")
    fuses = upload.get("fuses") if isinstance(upload.get("fuses"), dict) else {}
    port = str(data.get("port") or upload.get("port") or "").strip()
    if not port:
        port = _pick_default_serial_port()
    if not port and method in ("bootloader", "updi"):
        return jsonify({"success": False, "error": "No serial port found. Connect board and retry."}), 404
    if port and not re.match(r"^COM\d+$", port, flags=re.IGNORECASE):
        p = Path(port)
        if not p.exists():
            return jsonify({"success": False, "error": f"Serial port not found: {port}"}), 404

    if method == "bootloader":
        if not shutil.which("arduino-cli"):
            return jsonify({"success": False, "error": "arduino-cli not found in PATH."}), 500
        cmd = ["arduino-cli", "upload", "--fqbn", fqbn, "-p", port, str(project_dir)]
        try:
            rc, out = _run_cmd(cmd, cwd=project_dir, timeout_s=420)
            if rc == 0:
                return jsonify({
                    "success": True,
                    "board_id": board_id,
                    "method": method,
                    "port": port,
                    "fqbn": fqbn,
                    "returncode": rc,
                    "output": out,
                })
            return jsonify({
                "success": False,
                "board_id": board_id,
                "method": method,
                "port": port,
                "fqbn": fqbn,
                "returncode": rc,
                "error": "AVR bootloader upload failed.",
                "output": out,
            }), 500
        except subprocess.TimeoutExpired:
            return jsonify({"success": False, "error": "AVR bootloader upload timed out."}), 504

    if method not in ("isp", "updi"):
        return jsonify({"success": False, "error": f"Unsupported AVR upload method: {method}"}), 400

    if not shutil.which("arduino-cli"):
        return jsonify({"success": False, "error": "arduino-cli not found in PATH (required to produce .hex)."}), 500
    if not shutil.which("avrdude"):
        return jsonify({"success": False, "error": "avrdude not found in PATH."}), 500

    build_out = project_dir / "build"
    build_out.mkdir(parents=True, exist_ok=True)
    compile_cmd = ["arduino-cli", "compile", "--fqbn", fqbn, "--output-dir", str(build_out), str(project_dir)]
    try:
        rc_compile, out_compile = _run_cmd(compile_cmd, cwd=project_dir, timeout_s=420)
    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "error": "AVR compile timed out before flashing."}), 504
    if rc_compile != 0:
        return jsonify({
            "success": False,
            "board_id": board_id,
            "method": method,
            "fqbn": fqbn,
            "returncode": rc_compile,
            "error": "AVR compile failed before flashing.",
            "output": out_compile,
        }), 500

    hex_files = sorted(build_out.rglob("*.hex"))
    if not hex_files:
        return jsonify({"success": False, "error": f"No .hex file produced in {build_out}."}), 500
    hex_file = hex_files[0]

    flash_cmd = ["avrdude", "-p", mcu, "-c", programmer]
    if port:
        flash_cmd += ["-P", port]
    if baud not in (None, ""):
        try:
            flash_cmd += ["-b", str(int(baud))]
        except Exception:
            return jsonify({"success": False, "error": "upload.baud must be an integer."}), 400
    flash_cmd += ["-U", f"flash:w:{hex_file}:i"]
    for key in ("lfuse", "hfuse", "efuse", "lock"):
        val = str(fuses.get(key, "")).strip()
        if val:
            flash_cmd += ["-U", f"{key}:w:{val}:m"]

    try:
        rc_flash, out_flash = _run_cmd(flash_cmd, cwd=project_dir, timeout_s=300)
    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "error": "AVR flash timed out."}), 504
    if rc_flash != 0:
        return jsonify({
            "success": False,
            "board_id": board_id,
            "method": method,
            "programmer": programmer,
            "port": port,
            "mcu": mcu,
            "returncode": rc_flash,
            "error": "AVR flash failed.",
            "compile_output": out_compile,
            "flash_output": out_flash,
        }), 500

    return jsonify({
        "success": True,
        "board_id": board_id,
        "method": method,
        "programmer": programmer,
        "port": port,
        "mcu": mcu,
        "hex": str(hex_file),
        "compile_output": out_compile,
        "flash_output": out_flash,
    })


@app.route('/api/generate', methods=['POST'])
def generate_config():
    """Generate configuration files based on user selections"""
    config_data = request.get_json(silent=True) or {}
    config_data["enabled_drivers"] = config_gen._enabled_drivers_from_config(config_data)
    config_data["driver_config"] = config_gen._driver_config_from_config(config_data)
    config_data = config_gen._with_legacy_driver_keys(config_data)
    # Local configurator scope: market app selection is disabled by default.
    if not MARKET_FEATURES_ENABLED:
        config_data["market_apps"] = []

    required_fields = ['platform', 'board_id']
    for field in required_fields:
        if field not in config_data:
            return jsonify({'error': f'Missing required field: {field}'}), 400
    if not ("enabled_drivers" in config_data or "enabled_modules" in config_data):
        return jsonify({'error': "Missing required field: enabled_drivers"}), 400
    
    try:
        output_files = config_gen.save_configuration(config_data)
        
        return jsonify({
            'success': True,
            'message': 'Configuration files generated successfully',
            'files': output_files
        })
    
    except ValueError as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 400
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/api/preview/<config_type>', methods=['POST'])
def preview_config(config_type):
    """Preview generated configuration without saving"""
    config_data = request.get_json(silent=True) or {}
    config_data["enabled_drivers"] = config_gen._enabled_drivers_from_config(config_data)
    config_data["driver_config"] = config_gen._driver_config_from_config(config_data)
    config_data = config_gen._with_legacy_driver_keys(config_data)
    # Local configurator scope: market app selection is disabled by default.
    if not MARKET_FEATURES_ENABLED:
        config_data["market_apps"] = []
    
    try:
        target = config_gen._select_target(config_data)
        if config_type == 'basalt_config_h':
            backend = target.get("codegen", {}).get("backend") if target else None
            if backend == "basalt_codegen_pic16_xc8":
                board_id = config_data.get("board_id")
                board = config_gen.resolve_board(board_id)
                if not board:
                    raise ValueError("Board not found.")

                enabled_modules = config_gen._enabled_drivers_from_config(config_data)
                config_gen._validate_module_policy(target, enabled_modules)
                custom_pins = config_data.get("custom_pins") or {}
                module_config = config_gen._driver_config_from_config(config_data)
                pins = config_gen._merge_pins(board, custom_pins)
                config_gen._validate_pins(board, pins)

                content = render_pic16_preview(
                    board_id=board_id,
                    target_id=target.get("id", "pic16"),
                    enabled_modules=enabled_modules,
                    pins=pins,
                    module_config=module_config,
                    clock=(board.get("defaults") or {}).get("clock"),
                    fuses=(board.get("defaults") or {}).get("fuses"),
                )
            elif backend == "basalt_codegen_avr_arduino":
                board_id = config_data.get("board_id")
                board = config_gen.resolve_board(board_id)
                if not board:
                    raise ValueError("Board not found.")

                enabled_modules = config_gen._enabled_drivers_from_config(config_data)
                config_gen._validate_module_policy(target, enabled_modules)
                custom_pins = config_data.get("custom_pins") or {}
                module_config = config_gen._driver_config_from_config(config_data)
                pins = config_gen._merge_pins(board, custom_pins)
                config_gen._validate_pins(board, pins)
                clock = config_gen._avr_clock_from_config(config_data, board)
                fuses = config_gen._avr_fuses_from_config(config_data, board)

                content = render_avr_preview(
                    board_id=board_id,
                    target_id=target.get("id", "avr"),
                    enabled_modules=enabled_modules,
                    pins=pins,
                    module_config=module_config,
                    clock=clock,
                    fuses=fuses,
                )
            else:
                content = config_gen.generate_basalt_config_h(
                    config_data['board_id'],
                    config_data['enabled_drivers'],
                    config_data.get('custom_pins', {}),
                    config_data.get('platform'),
                    config_data.get('driver_config', {}),
                    config_data.get('applets', []),
                    config_data.get('market_apps', []),
                )
        elif config_type == 'sdkconfig':
            content = config_gen.generate_sdkconfig_defaults(
                config_data['board_id'],
                config_data['platform'],
                config_data['enabled_drivers']
            )
        else:
            return jsonify({'error': 'Invalid config type'}), 400
        
        return jsonify({
            'success': True,
            'content': content
        })
    
    except ValueError as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 400
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/')
def index():
    """Serve the HTML GUI"""
    return send_from_directory(str(TOOLS_DIR), 'basaltos_config_gui.html')


if __name__ == '__main__':
    print("BasaltOS Configuration Server")
    print(f"Boards directory: {BOARDS_DIR}")
    print(f"Drivers directory: {MODULES_DIR}")
    print(f"Platforms directory: {PLATFORMS_DIR}")
    print(f"Loaded {len(config_gen.boards)} boards")
    print(f"Loaded {len(config_gen.modules)} drivers")
    print(f"Loaded {len(config_gen.platforms)} platforms")
    print("\nStarting server on http://localhost:5000")
    app.run(debug=True, host='0.0.0.0', port=5000)
