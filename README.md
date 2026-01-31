# Basalt OS

Basalt OS is a lightweight, portable embedded OS focused on making **quick projects easy for non-engineers**. The long-term goal is an "Android-light" experience on small devices, starting with ESP32 and designed to scale upward.

## Current status (v0.x)
- CLI shell (`bsh`) over UART + TFT console (text mode)
- SPIFFS filesystem mounted at `/data`
- App install/run lifecycle (`install`, `run`, `remove`)
- Embedded MicroPython runtime + `basalt.*` APIs (GPIO, timer, LED)
- SPI SD card mount at `/sd` (FATFS) for app transfer
- Store-only zip packaging tool (`tools/pack_app.py`)

## Release notes (unreleased)
- Added SD card mount at `/sd` and FATFS long filename support.
- Added `mkdir`, `cp`, `rm`, `rm -r` in bsh for basic file management.
- Added store-only zip install workflow and host pack tool.
- Improved prompt readability (UART + TFT# BasaltOS

BasaltOS is a lightweight, portable embedded OS / application platform aimed at making **small hardware projects feel like installing and running apps**—even for non-engineers.

The long-term goal is an “Android-light” experience on microcontrollers: a friendly shell, installable apps, stable APIs, and a configuration system that scales from quick prototypes to real products.

---

## Status (v0.0.3)

What’s working today:

- **Basalt Shell (`bsh`)** over UART (and evolving toward richer UI targets)
- **Filesystem support**
  - SPIFFS internal flash (“storage” partition baked from `spiffs/`)
  - Optional SD card filesystem (board/module dependent)
- **App lifecycle in progress**
  - install/run/remove concepts
  - “store-only zip” packaging tools (`tools/pack_app.py`)
- **Embedded MicroPython runtime**
  - MicroPython is integrated as a submodule/runtime
  - `basalt.*` APIs exist and will expand over time
- **New configuration system**
  - **Boards + Modules + Platforms** and a **wizard** to generate build configuration

This release is about **workflow**: selecting a board + features cleanly, generating config, and building repeatably.

---

## Big idea: Configure once, build anywhere

BasaltOS is moving toward a model where you choose:

1. **Platform** (esp32, stm32, rp2040, …)
2. **Board variant** (specific devkit/module/display/etc.)
3. **Modules / features** (spi, uart, tft, filesystem, …)

…and BasaltOS generates:

- `config/generated/basalt_config.h` (compile-time feature switches + board pins)
- `config/generated/sdkconfig.defaults` (ESP-IDF defaults, layered by platform/board)

This is the “backend” that later becomes a GUI (Arduino/CubeMX-style), but it’s already usable today via CLI.

---

## Getting started (ESP-IDF / ESP32)

### 1) Clone + init submodules
```bash
git clone https://github.com/homer1013/BasaltOS
cd BasaltOS
git submodule update --init --recursive
```

### 2) Set up ESP-IDF environment
```bash
source tools/env.sh
```

### 3) Configure using the wizard
```bash
python tools/configure.py --wizard
```

This will guide you through:
- platform selection (e.g. `esp32`)
- board selection (e.g. `cyd_3248s035r`)
- module selection (e.g. `spi,tft,uart,fs_spiffs`)

…and generate configuration outputs into:
```
config/generated/
```

### 4) Build and flash
```bash
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build
idf.py -B build -p /dev/ttyUSB0 flash monitor
```

---

## Repository layout (current)

```
apps/
basalt_hal/
boards/
config/
docs/
main/
modules/
platforms/
runtime/
sdk/
spiffs/
tools/
```

---

## Roadmap

Near-term:
- Stabilize and expand `basalt.*` APIs
- Harden module gating and capability checks
- Improve CLI UX

Mid-term:
- GUI frontend for board/module selection
- Repeatable saved configurations

Long-term:
- Stable SDK feel
- Signed packages and permissions
- Multiple runtimes

---

## Contributing

If you’re new to the repo:
1. Run `python tools/configure.py --wizard`
2. Build and flash for your board
3. Open an issue with platform, board, and logs
 color).

### Format
- **Added**: new features
- **Changed**: behavior updates or refactors
- **Fixed**: bug fixes
- **Removed**: deprecated features

## Repository layout
```
/arch/              CPU-specific code (context switch, interrupts, atomics)
/hal/               SoC/peripheral abstractions
/kernel/            Scheduler, IPC, timers, core services
/drivers/           Device drivers (GPIO/I2C/SPI/UART/Display)
/runtime/           App runtimes (python/lua/wasm later)
/apps/              Example and test apps
/system/            Basalt system files (config, logs)
/data/              App data storage
/docs/              Specs and design notes
/tools/             Build/flash helpers
```

## Quick start
```
git clone https://github.com/homer1013/BasaltOS
cd BasaltOS
git submodule update --init --recursive
source tools/env.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Notes:
- Use `tools/board.sh <board>` to apply a board profile.
- `spiffs/` is baked into the `/data` storage partition.

## ESP-IDF bring-up (ESP32 CYD on /dev/ttyUSB0)
```
source tools/env.sh
tools/board.sh esp32-cyd
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Board switching
Use `tools/board.sh <board>` to copy a board profile into `sdkconfig.defaults` and `partitions.csv`.
See `boards/README.md` for profiles and status.

## SPIFFS apps
Files in `spiffs/` are baked into the `storage` partition at build time.
Example: `spiffs/hello.py` can be run with `run /data/hello.py`.

## MicroPython submodule
Basalt OS tracks MicroPython as a git submodule at `runtime/python/micropython`.

Initialize or update it:
```
git submodule update --init --recursive
git submodule update --remote --merge
```

If you change the submodule revision, commit the submodule pointer update in this repo.

## SD card apps
If the board has SD enabled, it mounts at `/sd`. You can install a store-only zip:
```
install /sd/blink_rgb.zip blink_rgb
```

## Developer docs
- `docs/third-party-app-dev.md` (app developer guide)
- `docs/dev-internal.md` (internal development guide)
- `docs/cli-bsh.md` (shell commands)
- `docs/runtime-micropython.md` (runtime details)
- `docs/api-basalt.md` (Basalt APIs)
- `docs/boards.md` (board profiles + pinouts)

## App SDK (templates)
Use `tools/new_app.py` to generate a minimal app skeleton under `apps/`:
```
python tools/new_app.py blink_rgb
```

## Next steps (internal)
1) Stabilize MicroPython embed and Basalt APIs.
2) Formalize UI/graphics API and touch input.
3) Add app permissions / resource limits.
4) Improve filesystem tooling and package signing.
