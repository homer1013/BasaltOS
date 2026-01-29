# Tools

## Available tools

- `env.sh` – set up ESP-IDF environment
- `board.sh` – apply a board profile (sdkconfig + partitions)
- `build.sh` – wrapper for idf.py build
- `flash.sh` – wrapper for idf.py flash
- `monitor.sh` – wrapper for idf.py monitor
- `gen_mpy_embed.sh` – build MicroPython embed sources
- `pack_app.py` – create store-only zip app packages
- `new_app.py` – create a new app skeleton (app.toml + main.py)

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
