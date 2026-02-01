#!/usr/bin/env python3
"""
BasaltOS Configuration GUI - Backend Server
Provides REST API for board/module configuration and generates config files
"""

import os
import json
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Any, Tuple
from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Configuration paths (adjust based on your BasaltOS structure)
BASALTOS_ROOT = Path(__file__).parent.parent
BOARDS_DIR = BASALTOS_ROOT / "boards"
MODULES_DIR = BASALTOS_ROOT / "modules"
PLATFORMS_DIR = BASALTOS_ROOT / "platforms"
CONFIG_OUTPUT_DIR = BASALTOS_ROOT / "config" / "generated"

# Reuse core configurator logic for correctness
sys.path.append(str(BASALTOS_ROOT / "tools"))
import configure as cfg


class ConfigGenerator:
    """Generate BasaltOS configuration files"""
    
    def __init__(self):
        self.boards = {}
        self.modules = {}
        self.platforms = {}
        self.load_all_configs()
    
    def load_all_configs(self):
        """Load all board, module, and platform configurations"""
        self.load_boards()
        self.load_modules()
        self.load_platforms()
    
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
        candidates.extend(list(BOARDS_DIR.glob("*.json")))
        candidates.extend(list(BOARDS_DIR.rglob("board.json")))
        candidates.extend([p for p in BOARDS_DIR.rglob("*.json") if p.parent != BOARDS_DIR])

        seen = set()
        for board_file in candidates:
            if board_file in seen:
                continue
            seen.add(board_file)

            if board_file.name.lower().startswith("readme"):
                continue

            try:
                with open(board_file, "r", encoding="utf-8") as f:
                    board_data = json.load(f)

                board_id, platform = derive_ids(board_file, board_data)
                if platform:
                    board_data["platform"] = platform

                self.boards[board_id] = board_data
            except Exception as e:
                print(f"Error loading board {board_file}: {e}")

    def load_modules(self):
        """Load module configurations from modules/ directory (recursively).

        Supported layouts:
          - modules/<id>.json
          - modules/<module_id>/module.json
        """
        if not MODULES_DIR.exists():
            print(f"Warning: Modules directory not found: {MODULES_DIR}")
            return

        candidates = []
        candidates.extend(list(MODULES_DIR.glob("*.json")))
        candidates.extend(list(MODULES_DIR.rglob("module.json")))
        candidates.extend([p for p in MODULES_DIR.rglob("*.json") if p.parent != MODULES_DIR])

        seen = set()
        for module_file in candidates:
            if module_file in seen:
                continue
            seen.add(module_file)

            if module_file.name.lower().startswith("readme"):
                continue

            try:
                with open(module_file, "r", encoding="utf-8") as f:
                    module_data = json.load(f)

                module_id = module_file.parent.name if module_file.name == "module.json" else module_file.stem
                self.modules[module_id] = module_data
            except Exception as e:
                print(f"Error loading module {module_file}: {e}")

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


    def get_boards_by_platform(self, platform_id: str):
        """Return list of boards matching the given platform_id."""
        out = []
        for board_id, data in self.boards.items():
            plat = data.get("platform")
            if plat == platform_id:
                # ensure id is present for the GUI
                item = dict(data)
                item.setdefault("id", board_id)
                out.append(item)
        # stable ordering
        out.sort(key=lambda x: x.get("name", x.get("id", "")))
        return out


    def get_available_modules(self, platform_id: str | None = None):
        """Return list of modules, optionally filtered by platform.

        If a module declares supported platforms (e.g. 'platforms' or 'supported_platforms'),
        it will be filtered accordingly. If no such field exists, the module is treated as
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


    def _resolve(self, config_data: dict) -> Tuple[str | None, cfg.BoardProfile | None, List[str], Dict[str, cfg.Module]]:
        platform = config_data.get("platform")
        board_id = config_data.get("board_id")
        enabled_modules = config_data.get("enabled_modules", []) or []

        modules = cfg.discover_modules(BASALTOS_ROOT / "modules")
        boards = cfg.discover_boards(BASALTOS_ROOT, platform)

        board_profile = None
        allow = None
        if platform and board_id:
            try:
                board_profile, allow = cfg.select_board_noninteractive(boards, platform, board_id)
            except cfg.ConfigError as ex:
                raise ValueError(str(ex))

        enabled_list = cfg.resolve_modules(modules, set(enabled_modules), allow=allow)
        return platform, board_profile, enabled_list, modules


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


    def generate_basalt_config_h(self, board_id: str | None, enabled_modules: List[str], custom_pins: dict | None, platform: str | None) -> str:
        platform, board_profile, enabled_list, modules = self._resolve({
            "platform": platform,
            "board_id": board_id,
            "enabled_modules": enabled_modules,
        })

        board_define_name = board_profile.id if board_profile else board_id
        board_data = self._board_data_with_pins(board_profile, custom_pins)

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
            return out_path.read_text(encoding="utf-8")


    def generate_sdkconfig_defaults(self, board_id: str | None, platform: str | None, enabled_modules: List[str]) -> str:
        platform, board_profile, enabled_list, modules = self._resolve({
            "platform": platform,
            "board_id": board_id,
            "enabled_modules": enabled_modules,
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
        platform, board_profile, enabled_list, modules = self._resolve(config_data)
        custom_pins = config_data.get("custom_pins") or {}

        board_define_name = board_profile.id if board_profile else config_data.get("board_id")
        board_id = board_profile.id if board_profile else config_data.get("board_id")
        board_dir = board_profile.board_dir if board_profile else None
        board_data = self._board_data_with_pins(board_profile, custom_pins)

        outdir = CONFIG_OUTPUT_DIR
        outdir.mkdir(parents=True, exist_ok=True)

        basalt_h = outdir / "basalt_config.h"
        features_j = outdir / "basalt.features.json"
        sdk_defaults = outdir / "sdkconfig.defaults"
        config_json = outdir / "basalt_config.json"

        cfg.emit_basalt_config_h(
            out_path=basalt_h,
            enabled_modules=enabled_list,
            modules=modules,
            platform=platform,
            board_define_name=board_define_name,
            board_data=board_data,
        )
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
        payload["resolved_modules"] = enabled_list
        config_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

        return {
            "basalt_config.h": str(basalt_h),
            "basalt.features.json": str(features_j),
            "sdkconfig.defaults": str(sdk_defaults),
            "basalt_config.json": str(config_json),
        }


# Instantiate the config generator (loads boards/modules/platforms)
config_gen = ConfigGenerator()

@app.route('/api/platforms', methods=['GET'])
def get_platforms():
    """Get list of available platforms"""
    platforms = []
    for platform_id, platform_data in config_gen.platforms.items():
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
    boards = config_gen.get_boards_by_platform(platform)
    return jsonify(boards)


@app.route('/api/modules', methods=['GET'])
def get_modules():
    """Get available modules"""
    platform = request.args.get('platform')
    modules = getattr(config_gen, 'get_available_modules', lambda p: [dict(v, id=k) for k,v in config_gen.modules.items()])(platform)
    return jsonify(modules)


@app.route('/api/board/<board_id>', methods=['GET'])
def get_board_details(board_id):
    """Get detailed information about a specific board"""
    board = config_gen.boards.get(board_id)
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
        'pins': board.get('pins', {}),
        'capabilities': board.get('capabilities', [])
    })


@app.route('/api/generate', methods=['POST'])
def generate_config():
    """Generate configuration files based on user selections"""
    config_data = request.json
    
    required_fields = ['platform', 'board_id', 'enabled_modules']
    for field in required_fields:
        if field not in config_data:
            return jsonify({'error': f'Missing required field: {field}'}), 400
    
    try:
        output_files = config_gen.save_configuration(config_data)
        
        return jsonify({
            'success': True,
            'message': 'Configuration files generated successfully',
            'files': output_files
        })
    
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/api/preview/<config_type>', methods=['POST'])
def preview_config(config_type):
    """Preview generated configuration without saving"""
    config_data = request.json
    
    try:
        if config_type == 'basalt_config_h':
            content = config_gen.generate_basalt_config_h(
                config_data['board_id'],
                config_data['enabled_modules'],
                config_data.get('custom_pins', {}),
                config_data.get('platform')
            )
        elif config_type == 'sdkconfig':
            content = config_gen.generate_sdkconfig_defaults(
                config_data['board_id'],
                config_data['platform'],
                config_data['enabled_modules']
            )
        else:
            return jsonify({'error': 'Invalid config type'}), 400
        
        return jsonify({
            'success': True,
            'content': content
        })
    
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/')
def index():
    """Serve the HTML GUI"""
    return send_from_directory('.', 'basaltos_config_gui.html')


if __name__ == '__main__':
    print("BasaltOS Configuration Server")
    print(f"Boards directory: {BOARDS_DIR}")
    print(f"Modules directory: {MODULES_DIR}")
    print(f"Platforms directory: {PLATFORMS_DIR}")
    print(f"Loaded {len(config_gen.boards)} boards")
    print(f"Loaded {len(config_gen.modules)} modules")
    print(f"Loaded {len(config_gen.platforms)} platforms")
    print("\nStarting server on http://localhost:5000")
    app.run(debug=True, host='0.0.0.0', port=5000)
