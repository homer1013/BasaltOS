# BasaltOS Local Configurator

This is the local development configurator for BasaltOS.

It is intended for developers working from a cloned repository. It is not the
future hosted platform.

## Scope (what this tool is for)

- Board discovery and selection from local `boards/**/board.json`
- Driver discovery and selection from local `modules/**/module.json`
- Pin assignment validation and conflict detection
- Configuration preview and file generation
- Local helper flows for development (templates, flash/monitor helpers, DIY board metadata save)

## Out of scope (not in this local tool)

- User accounts/profiles
- Cloud project storage
- App marketplace/catalog management UI
- Payment or social features

## Architecture

- Frontend: `tools/basaltos_config_gui.html`
- Backend: `tools/basaltos_config_server.py`

## Quick Start

1. Install dependencies:

```bash
pip install flask flask-cors
```

2. Start server:

```bash
python tools/basaltos_config_server.py
```

3. Open UI:

- `http://127.0.0.1:5000`

## Local API Endpoints

- `GET /api/platforms`
- `GET /api/board-taxonomy`
- `GET /api/board-taxonomy/options`
- `GET /api/board-taxonomy/meta`
- `GET /api/boards/<platform>`
- `GET /api/drivers?platform=<platform>`
- `GET /api/board/<board_id>`
- `GET /api/sync/export-preview`
- `POST /api/generate`
- `POST /api/preview/<config_type>`

## Expected Outputs

Generation writes config artifacts such as:

- `config/generated/basalt_config.h`
- `config/generated/sdkconfig.defaults`
- `config/generated/basalt.features.json`

## Baseline Validation Commands

```bash
# One-command hardening bundle (recommended before push)
bash tools/tests/main_hardening_smoke_bundle.sh

# Optional focused guard: block staged local/generated artifacts
bash tools/tests/staged_artifact_guard.sh

# Optional docs drift guard: board catalog stays in sync
bash tools/tests/board_catalog_drift_smoke.sh

# Optional taxonomy guard: explicit manufacturer/architecture/family fields
bash tools/tests/board_taxonomy_fields_smoke.sh

# CLI metadata baseline
python3 tools/validate_metadata.py

# Configurator API baseline (with server running)
curl --fail-with-body -sS http://127.0.0.1:5000/api/platforms
curl --fail-with-body -sS http://127.0.0.1:5000/api/boards/esp32
curl --fail-with-body -sS "http://127.0.0.1:5000/api/drivers?platform=esp32"
```

## Notes

If you are looking for hosted marketplace/account/community capabilities, those
belong in the future `basaltos.io` platform codebase, not this local repo tool.

## Local Data Workspace

Local-only user data and caches should live under the workspace contract in:

- `docs/LOCAL_DATA_WORKSPACE.md`

Quick migration helper:

```bash
python3 tools/local_data_migrate.py --dry-run
python3 tools/local_data_migrate.py --apply
```

## Local Mode Guard Checks

Verify local-mode scope enforcement:

```bash
# UI guard test (requires playwright installed)
node tools/e2e/local_mode_nav_guard.js
```

Expected:
- guard test exits 0
