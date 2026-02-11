# Tools

## Available tools

- `env.sh` – set up ESP-IDF environment
- `board.sh` – list/select board profiles; applies profile-ready boards (sdkconfig + partitions)
- `validate_metadata.py` – validate board/module JSON schema and cross-references
- `build.sh` – wrapper for idf.py build
- `flash.sh` – wrapper for idf.py flash
- `monitor.sh` – wrapper for idf.py monitor
- `pic16_curiosity_nano_run.sh` – PIC16F13145 Curiosity Nano helper (XC8 compile + drag-and-drop flash)
- `gen_mpy_embed.sh` – build MicroPython embed sources
- `pack_app.py` – create store-only zip app packages
- `validate_app.py` – validate app folders/zip packages before upload/install
- `new_app.py` – create a new app skeleton (app.toml + main.py)
- `basaltos_config_server.py` – local configurator backend + App Market API
- `platformio/bootstrap_from_features.py` – generate a PlatformIO phase-1 starter config from `config/generated/basalt.features.json`
- `release_sync_check.py` – validate release/tag/changelog alignment across BasaltOS repos
- `release_sync_update.py` – update release sync status rows in one command

## new_app.py

Create a new app under `apps/`:
```
python tools/new_app.py blink_rgb
```

Create a new app under a custom folder:
```
python tools/new_app.py blink_rgb --dest my_apps
```

Set a display title for app.toml:
```
python tools/new_app.py blink_rgb --title "Blink RGB"
```

## validate_app.py

Validate an app folder:
```
python tools/validate_app.py apps/example.app
```

Validate a zip package:
```
python tools/validate_app.py dist/example.zip
```

Optional syntax check (best-effort with CPython):
```
python tools/validate_app.py apps/example.app --check-syntax
```

## pack_app.py

Pack an app into a store-only zip:
```
python tools/pack_app.py apps/example.app dist/example.zip
```

Pack with syntax validation:
```
python tools/pack_app.py apps/example.app dist/example.zip --check-syntax
```

## App Market API (local)

When running `python tools/basaltos_config_server.py`, these endpoints are available:

- `GET /api/market/catalog?platform=<id>` – list curated market apps
- `GET /api/market/download/<app_id>?platform=<id>` – download app package zip
- `POST /api/market/upload` – upload app package zip and upsert local catalog entry
  - multipart fields:
    - `package` (required zip)
    - `id`, `name`, `description`, `version`, `author`, `platforms` (optional metadata)
  - server validates package structure + app entry before accepting upload

## CLI Regression Tests

Metadata validator regression (report mode + required-field enforcement):

```bash
bash tools/tests/validate_metadata_report_regression.sh
```

Invalid-board hard-fail regression guard:

```bash
bash tools/tests/configure_invalid_board_regression.sh
```

Negative-path CLI validation (template/platform/driver/input failures):

```bash
bash tools/tests/configure_negative_paths.sh
```

Multi-board CLI smoke run (all discovered boards):

```bash
bash tools/tests/configure_smoke_multi_board.sh
```

Deterministic generation check (normalizes `generated_utc` JSON field before diff):

```bash
bash tools/tests/configure_deterministic_outputs.sh
```

Configurator API smoke test:

```bash
bash tools/tests/configurator_api_smoke.sh
```

Local-mode UI/API guard test (market/profile hidden + market API disabled):

```bash
node tools/e2e/local_mode_nav_guard.js
```


## PlatformIO Phase-1 Bootstrap

```bash
python3 tools/platformio/bootstrap_from_features.py
```

Override board id if needed:

```bash
python3 tools/platformio/bootstrap_from_features.py --pio-board esp32-c3-devkitm-1
```

PlatformIO phase-1 contract smoke (bootstrap from generated features):

```bash
bash tools/tests/platformio_phase1_contract.sh
```

BME280 driver configure smoke:

```bash
bash tools/tests/configure_bme280_smoke.sh
```

MCP2515 driver configure smoke:

```bash
bash tools/tests/configure_mcp2515_smoke.sh
```


Release sync checker (self-only, CI-safe):

```bash
python3 tools/release_sync_check.py --self-only --version v0.1.0
```

Cross-repo release sync check (local workspace with sibling repos):

```bash
python3 tools/release_sync_check.py --version v0.1.0
```

Release sync checker smoke test:

```bash
bash tools/tests/release_sync_check_smoke.sh
```


Release sync status updater:

```bash
python3 tools/release_sync_update.py --version v0.1.1 --all-status in_progress
python3 tools/release_sync_update.py --version v0.1.1 --main-status synced --platform-status synced --platformio-status in_progress
```

Release sync updater smoke test:

```bash
bash tools/tests/release_sync_update_smoke.sh
```

## PIC16 Curiosity Nano Helper

End-to-end helper for local PIC16 bench flow (XC8 + DFP + drag-drop flash):

```bash
bash tools/pic16_curiosity_nano_run.sh
```

Compile only (no flash):

```bash
bash tools/pic16_curiosity_nano_run.sh --no-flash
```
