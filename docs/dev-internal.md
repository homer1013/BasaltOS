# Basalt OS Internal Development Guide

This document is for contributors working on Basalt OS itself (not app authors).
It summarizes the current architecture, build flow, and common gotchas.

## Architecture at a glance

- **Shell (bsh):** `main/app_main.c`
  - Command parsing and dispatch
  - App install/run/remove
  - Filesystem commands (`ls`, `cp`, `rm`, `mkdir`, etc)
- **TFT console:** `main/tft_console.c`
  - UART output mirrored to TFT
  - Per‑character color buffer
- **MicroPython embed:** `runtime/python/micropython_embed`
  - Minimal port with `basalt.*` APIs
- **Storage:**
  - SPIFFS mounted at `/data`
  - App namespace `/apps` mapped to `/data/apps`
  - Optional SD card mounted at `/sd` (FATFS)

## Build & flash

```
source tools/env.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Notes:
- `sdkconfig.defaults` is applied only when generating a new `sdkconfig`.
- Board profiles are applied with `tools/board.sh <board>`.
- The SPIFFS image is generated from `spiffs/` into `storage.bin`.

## Filesystem conventions

- **SPIFFS**: `/data`
- **Apps**: `/apps` (virtual), `/data/apps` (real)
- **SD card**: `/sd` when available (FATFS)

Shell paths:
- Always run apps from `/apps/<name>` or `/data/...`
- `install` supports folders and store‑only zip files

## Basalt APIs (MicroPython)

Current exports:
- `basalt.gpio.mode(pin, mode)`
- `basalt.gpio.set(pin, value)`
- `basalt.gpio.get(pin)`
- `basalt.timer.sleep_ms(ms)`
- `basalt.led.set(r, g, b)`
- `basalt.led.off()`

## App packaging

`tools/pack_app.py` produces a store‑only zip for app installs.
This avoids runtime decompression to keep memory usage low.

## SD card notes

- SD card uses VSPI pins by default (MOSI=23, MISO=19, SCLK=18, CS=5)
- FATFS LFN is enabled; users may still see short 8.3 names in listings

## Prompt coloring

UART prompt uses ANSI color.
TFT prompt uses `tft_console_set_color()` before drawing the prompt.

## Common pitfalls

- `sdkconfig` overrides defaults; delete it to re‑apply defaults.
- SPIFFS path lengths are limited; keep names short.
- FAT short names may appear; use the visible name in commands.
- Run `idf.py fullclean` after large config changes.

## Testing checklist (manual)

- Boot: TFT console shows prompt, UART works.
- Filesystem: `/data` mounted, `/apps` maps to `/data/apps`.
- SD: card mounts at `/sd`, can `ls` and `cp` files.
- Install: `install /sd/app.zip app` then `run app`.
- Remove: `remove app`, `rm -r` directory deletion works.

