# Basalt OS 3rd‑Party App Developer Guide

This document explains how to build and run third‑party apps on Basalt OS.
It is written for MicroPython apps running on ESP32 targets (Basalt OS v0.x).

## Overview

Basalt OS is a lightweight embedded OS that provides:
- A CLI shell (bsh) over UART/TFT
- A SPIFFS-backed filesystem mounted at `/data`
- An app namespace at `/apps` (mapped to `/data/apps`)
- A MicroPython runtime with a Basalt module (`basalt.*`) for hardware access
- A built-in TFT console on supported boards (text output mirrored to display)
- App packaging via folder or store-only zip (`install`)

Apps are just folders (or files) placed under `/apps`. The OS runs them using
MicroPython when you call `run`.

## Requirements

### Hardware
- ESP32-class device (Basalt OS currently targets ESP32)
- A Basalt‑compatible board profile (see `docs/boards.md`)
- Optional display (Basalt OS supports TFT console on supported boards)

### Firmware
- Basalt OS built and flashed with:
  - SPIFFS enabled
  - MicroPython embed runtime enabled
  - Basalt module enabled (GPIO + timer)

### Software (host machine)
- ESP‑IDF v5.x environment (used to build/flash the OS)
- Python 3.x (ESP‑IDF tools)

## Filesystem & App Layout

Basalt OS mounts SPIFFS at:
- `/data`

App space is:
- `/apps` (virtual)
- `/data/apps` (real)

### Minimal app layout
```
/apps/<app_name>/
  app.toml      (optional)
  main.py|main.lua   (required if app.toml missing)
```

### app.toml (optional)
If present, Basalt OS reads the `entry` field to pick the entrypoint.

Example:
```
name = "Blink"
version = "0.1.0"
entry = "main.py"
runtime = "python"
```

Supported runtime values:
- `python` (default, alias: `micropython`)
- `lua`

Reference sample apps:
- `apps/demo.app` (python)
- `apps/lua_hello.app` (lua)
- `apps/lua_blink.app` (lua)

Entrypoint conventions:
- if `runtime = "python"` and `entry` is omitted, default is `main.py`
- if `runtime = "lua"` and `entry` is omitted, default is `main.lua`
- if `app.toml` is missing, loader defaults to `main.py` and falls back to `main.lua` if present

## Basalt MicroPython APIs

Basalt OS exposes Basalt modules inside MicroPython:

```
import basalt

basalt.gpio.mode(pin, mode)   # mode: 0=input, 1=output
basalt.gpio.set(pin, value)   # value: 0/1
basalt.gpio.get(pin)          # returns 0/1
basalt.timer.sleep_ms(ms)
basalt.led.set(r, g, b)       # r/g/b are 0 or 1
basalt.led.off()
```

Optional modules (driver-gated):
```python
import basalt.ui as ui        # TFT text/widget bridge
import basalt.rtc             # rtc.available(), rtc.now()
import basalt.ssd1306         # OLED debug drawing helpers
```

For latest API surface, see `docs/api-basalt.md`.

## Display / UI (current state)

Basalt OS provides a **text console** on supported TFT boards, and stdout is mirrored there.
It also provides a **UI API** (`basalt.ui`) with screen/label/button primitives rendered through
the TFT text bridge in current firmware. This keeps app code stable while touch/graphics expand.

### UI API (current behavior)
The API below renders text labels/buttons at requested coordinates when TFT console is active.

```python
import basalt.ui as ui

# Create a screen
screen = ui.screen()

# Create a button
btn = ui.button("Toggle LED", x=20, y=30, w=120, h=40)

def on_press():
    # Example hook into basalt.gpio
    # basalt.gpio.set(2, 1) ...
    pass

btn.on_press(on_press)
screen.add(btn)
screen.show()
```

Notes:
- `ui.screen()` returns a root container
- widgets (button/label/slider/list) attach to a screen
- events are callback-based
- touch input will map to widget events

### Longer UI example
This runs today with text rendering; interaction callbacks remain placeholder-level.

```python
import basalt
import basalt.ui as ui

screen = ui.screen()
ui.set_title("Demo")

label = ui.label("LED: OFF", x=10, y=10)
button = ui.button("Toggle LED", x=10, y=40, w=120, h=30)

state = {"on": False}

def on_press():
    state["on"] = not state["on"]
    label.set_text("LED: ON" if state["on"] else "LED: OFF")
    if state["on"]:
        basalt.led.set(1, 0, 0)
    else:
        basalt.led.off()

button.on_press(on_press)
screen.add(label)
screen.add(button)
screen.show()

while True:
    basalt.timer.sleep_ms(250)
```

### Board/display configuration
Display drivers and pin mappings are **board-specific**. Those live in board
profiles and the TFT driver setup, not inside apps. App developers should
assume the display is already initialized by the OS.

For details, see `docs/boards.md`.

### LED configuration
RGB LED pins and polarity are also **board-specific** and configured in the
board profile. App developers should use `basalt.led.*` instead of hardcoding
GPIO pins.

## CLI Workflow

### List apps
```
apps
```

### Help output
```
help
help -commands
```

### Run an app
```
run <app_name>
```

### Run a script directly
```
run /data/path/to/script.py
```

### Edit a script on-device
```
edit /apps/<app_name>/main.py
```
Type lines and finish with:
- `.save` to write
- `.quit` to abort

### Install/Remove apps
```
install /data/source_app_folder [name]
remove <app_name>
```

`install` copies a folder into `/apps/<name>`.
If `name` is omitted, the source folder name is used.

### Delete files and folders
```
rm /data/hello.py
rm /sd/blink_rgb.zip
rm -r /sd/.Trash-1000
```

`rm -r` is required for directories.

### Install from a zip (store-only)
Basalt OS supports **store-only** zip files (no compression).  
The zip should contain a single app at its root (for example: `demo/main.py`, `demo/app.toml`).

Install with:
```
install /data/demo.zip demo
```

Note: compressed zip files are not supported yet.

### Packaging tool (recommended)
To make a compatible store-only zip, use the host tool:
```
python tools/pack_app.py path/to/app_dir out.zip
```

Example:
```
python tools/pack_app.py apps/demo demo.zip
install /data/demo.zip demo
```

The packer:
- forces store-only (no compression)
- ensures a single top-level folder inside the zip
- validates manifest/entry before packing

### Validate before upload/install (recommended)
Validate a local app folder:
```
python tools/validate_app.py apps/demo.app
```

Validate a zip package:
```
python tools/validate_app.py demo.zip
```

Optional CPython syntax check:
```
python tools/validate_app.py apps/demo.app --check-syntax
```

### Transfer via microSD (recommended)
For now, use a microSD card to move apps between your computer/phone and the device.
Copy your app (folder or store-only zip) to the card, insert it, and install from
the mounted path.

Example workflow (mount point varies by board):
1) Insert the SD card and boot Basalt OS.
2) In bsh, list root to find the mount (look for `sd`, `sdcard`, or similar):
```
ls /
```
3) If you see (for example) `/sd`, copy your app zip to the card on your PC and then:
```
install /sd/blink_rgb.zip blink_rgb
```

Notes:
- Default SPI SD pins (VSPI): MOSI=23, MISO=19, SCLK=18, CS=5.
- If no SD mount appears, the board profile may not have SD enabled yet.
- Check `docs/boards.md` or your board config for the expected mount path.
- FAT filesystems may show **short 8.3 names** (e.g. `BLINK_~1.ZIP`).  
  If that happens, just use the short name in `install`/`rm`.

### Copying apps from ESP32 to SD
You can copy files or entire apps to the SD card directly from bsh.

Copy a single file:
```
cp /data/apps/demo/main.py /sd/demo_main.py
```

Copy a whole app (SPIFFS flat layout) into a folder on SD:
```
mkdir /sd/apps
cp /apps/demo /sd/apps/demo
```

This copies all files under `/apps/<name>` into `/sd/apps/<name>`.

### Move or rename
```
mv /sd/blink_rgb.zip /sd/blink_rgb_1.zip
mv -r /sd/apps/demo /sd/apps/demo_backup
mv -R /sd/apps/demo /sd/apps/demo_backup
```

Move an app prefix (SPIFFS flat layout):
```
mv /apps/demo /sd/apps/demo
```

## Development Tips

- Keep app file names short. SPIFFS has path length limits.
- Use `/apps/<name>` so your app is portable across boards.
- Prefer GPIO numbers rather than board silkscreen labels.
- If you need UI, use `basalt.ui` (portable path) instead of touching panel drivers directly.
- If your board enables SSD1306, use `basalt.ssd1306` only as a secondary/debug surface.
- Do not reinitialize shared display buses in app code; the OS owns those resources.

## Demo App Example

Create a simple RGB blink app:

```
import basalt

pins = (4, 16, 17)
for p in pins:
    basalt.gpio.mode(p, 1)
    basalt.gpio.set(p, 0)

for _ in range(3):
    basalt.gpio.set(4, 1)
    basalt.timer.sleep_ms(150)
    basalt.gpio.set(4, 0)

    basalt.gpio.set(16, 1)
    basalt.timer.sleep_ms(150)
    basalt.gpio.set(16, 0)

    basalt.gpio.set(17, 1)
    basalt.timer.sleep_ms(150)
    basalt.gpio.set(17, 0)
```

## Compatibility Notes

- Basalt OS is evolving. Stick to the Basalt APIs (`basalt.*`) when possible.
- Direct ESP‑IDF hardware access from Python is not supported.
- Keep memory usage small; ESP32 RAM is limited.

## Where to Look Next

- `docs/cli-bsh.md` (shell commands)
- `docs/runtime-micropython.md` (MicroPython runtime details)
- `docs/api-basalt.md` (Basalt API reference)
