# BasaltOS Local Configurator

This is the local development configurator for BasaltOS.

It is intended for developers working from a cloned repository. It is not the
future hosted platform.

## Scope (what this tool is for)

- Board discovery and selection from local `boards/**/board.json`
- Driver discovery and selection from local `modules/**/module.json`
- Pin assignment validation and conflict detection
- Configuration preview and file generation

## Out of scope (not in this local tool)

- User accounts/profiles
- Cloud project storage
- Hosted marketplace/community features
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
- `GET /api/boards/<platform>`
- `GET /api/drivers?platform=<platform>`
- `GET /api/board/<board_id>`
- `POST /api/generate`
- `POST /api/preview/<config_type>`

## Expected Outputs

Generation writes config artifacts such as:

- `config/generated/basalt_config.h`
- `config/generated/sdkconfig.defaults`
- `config/generated/basalt.features.json`

## Baseline Validation Commands

```bash
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


## Market Endpoint Toggle

Local market endpoints are disabled by default in local configurator mode.

To re-enable for explicit local testing:

```bash
BASALTOS_LOCAL_MARKET_ENABLED=1 python tools/basaltos_config_server.py
```

## Local Mode Guard Checks

Verify local-mode scope enforcement:

```bash
# API should return 404 when market endpoints are disabled by default
curl -sS -w "\nHTTP_CODE:%{http_code}\n" "http://127.0.0.1:5000/api/market/catalog?platform=esp32"

# UI guard test (requires playwright installed)
node tools/e2e/local_mode_nav_guard.js
```

Expected:
- market catalog request returns HTTP 404 with disabled-in-local-mode message
- guard test exits 0

