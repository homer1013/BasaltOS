# BasaltOS

BasaltOS is a lightweight, portable embedded OS / application platform aimed at making **small hardware projects feel like installing and running apps**—even for non-engineers.

The long-term goal is an “Android-light” experience on microcontrollers: a friendly shell, installable apps, stable APIs, and a configuration system that scales from quick prototypes to real products.

---

## Status (v0.0.5)

What’s working today:

- **Basalt Shell (`bsh`)** over UART + TFT console output
  - `help -commands` with clearer usage text
  - improved path handling (`cd`, virtual roots, app/dev roots)
  - app commands: `run`, `run_dev`, `stop`, `kill`, `apps`, `apps_dev`
  - diagnostics: `led_test`, `devcheck`
- **Filesystem support**
  - SPIFFS internal flash (“storage” partition baked from `spiffs/`)
  - Optional SD card filesystem (board/module dependent)
- **App lifecycle (early)**
  - install/run/remove concepts
  - “store-only zip” packaging tools (`tools/pack_app.py`)
  - dev app workflow under `dev/apps_dev` with shell execution via `run_dev`
- **Embedded MicroPython runtime**
  - MicroPython is integrated as a submodule/runtime
  - `basalt.*` APIs exist and are expanding
  - UI stub now draws basic text/widgets through TFT console bridge
- **Configuration system (CLI + Web)**
  - **Boards + Modules + Platforms** model
  - CLI wizard: `python tools/configure.py --wizard`
  - Local web configurator: `python tools/basaltos_config_server.py`
  - generated outputs:
    - `config/generated/basalt_config.h`
    - `config/generated/basalt.features.json`
    - `config/generated/sdkconfig.defaults`
- **Multi-target backend groundwork**
  - runtime-capable flow for ESP32/IDF boards
  - generated-firmware profile flow started for constrained targets (PIC/AVR direction)

This release is about **workflow + foundation**: select board/features cleanly, generate config repeatably, run apps/dev-apps faster, and start a cross-target generation model.

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

### 5) Optional: run the local web configurator
```bash
python tools/basaltos_config_server.py
```

Then open:
```
http://localhost:5000
```

This is intended for local workflow right now (no hosting/deployment required).

---

## Local dev apps workflow (ESP32 shell)

Dev app examples live in:
```
dev/apps_dev/
```

Typical flow:
1. copy a dev app onto device storage (`/apps_dev`) or SD (`/sd/apps_dev`)
2. from shell, run:
   - `apps_dev`
   - `run_dev <name>`
   - `kill` (to stop a running app)

For hardware sanity checks:
- `led_test` (or `led_test <pin>`)
- `devcheck` / `devcheck full`

---

## Repository layout (current)

```
apps/
basalt_hal/
boards/
config/
dev/
docs/
main/
modules/
platforms/
runtime/
targets/
sdk/
spiffs/
tools/
```

---

## Roadmap

Near-term:
- Stabilize and expand `basalt.*` APIs (UI, IO, runtime ergonomics)
- Continue shell UX polish and command safety
- Harden module gating + board capability constraints
- Complete board metadata (pins, flash/ram, LED mode/polarity, module support)

Mid-term:
- Full-featured configurator UX (validation, conflict hints, profile export/import)
- Better generated-project outputs for non-runtime targets
- Repeatable saved configurations and build profiles

Long-term:
- Stable SDK feel
- Signed packages and permissions
- Multiple runtimes
- Hosted configurator + broader target support

---

## Contributing

If you’re new to the repo:
1. Run `python tools/configure.py --wizard`
2. Build and flash for your board
3. Open an issue with platform, board, and logs
