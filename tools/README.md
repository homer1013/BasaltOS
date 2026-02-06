# Tools

## Available tools

- `env.sh` – set up ESP-IDF environment
- `board.sh` – list/select board profiles; applies profile-ready boards (sdkconfig + partitions)
- `validate_metadata.py` – validate board/module JSON schema and cross-references
- `build.sh` – wrapper for idf.py build
- `flash.sh` – wrapper for idf.py flash
- `monitor.sh` – wrapper for idf.py monitor
- `gen_mpy_embed.sh` – build MicroPython embed sources
- `pack_app.py` – create store-only zip app packages
- `new_app.py` – create a new app skeleton (app.toml + main.py)
- `basaltos_config_server.py` – local configurator backend + App Market API

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

## App Market API (local)

When running `python tools/basaltos_config_server.py`, these endpoints are available:

- `GET /api/market/catalog?platform=<id>` – list curated market apps
- `GET /api/market/download/<app_id>?platform=<id>` – download app package zip
- `POST /api/market/upload` – upload app package zip and upsert local catalog entry
  - multipart fields:
    - `package` (required zip)
    - `id`, `name`, `description`, `version`, `author`, `platforms` (optional metadata)
