# BasaltOS
[![Configurator CI](https://github.com/homer1013/BasaltOS/actions/workflows/configurator-ci.yml/badge.svg)](https://github.com/homer1013/BasaltOS/actions/workflows/configurator-ci.yml)
[![Sponsor](https://img.shields.io/badge/Sponsor-GitHub%20Sponsors-30363d?logo=github)](https://github.com/sponsors/homer1013)

BasaltOS is a lightweight, portable embedded OS / application platform aimed at making **small hardware projects feel like installing and running apps**—even for non-engineers.

The long-term goal is an “Android-light” experience on microcontrollers: a friendly shell, installable apps, stable APIs, and a configuration system that scales from quick prototypes to real products.

Copyright © 2026 Homer Morrill

---

## Project Philosophy

BasaltOS is an open-source embedded operating system and application platform designed to make embedded development more modular, approachable, and scalable.

The BasaltOS **core will remain open and free to use**. Development is supported through optional donations and, in the future, hosted tools and commercial offerings built **around** the core, not by restricting access to it.

The goal is to empower experimentation, education, and real-world products without lock-in.

## What Is BasaltOS Core?

BasaltOS Core includes:
- The BasaltOS kernel and runtime
- Hardware Abstraction Layer (HAL)
- Platform ports (e.g. ESP32, AVR)
- Core shell and system services
- Reference applications and examples
- Build system and tooling required to use the core

BasaltOS Core is fully open source and licensed under the Apache License 2.0.

## The BasaltOS Ecosystem

The BasaltOS ecosystem may include:
- Hosted tools (e.g. web flashing, configuration, CI builds)
- Optional commercial services or integrations
- Hardware kits or certified boards
- Educational material and documentation

These components may be offered under separate terms.

## Sponsor BasaltOS

If BasaltOS helps your work, you can support development via GitHub Sponsors:

- https://github.com/sponsors/homer1013

Sponsorship helps fund core maintenance, board bring-up, docs, CI time, and release quality work.

---

## Status (v0.1.0)

What’s working today:

- **Basalt Shell (`bsh`)** over UART + TFT console output
  - `help -commands` with clearer usage text
  - improved path handling (`cd`, virtual roots, app/dev roots)
  - app commands: `run`, `run_dev`, `stop`, `kill`, `apps`, `apps_dev`
  - diagnostics: `led_test`, `devcheck`, `drivers`
- **Filesystem support**
  - SPIFFS internal flash (“storage” partition baked from `spiffs/`)
  - Optional SD card filesystem (board/driver dependent)
- **App lifecycle (early)**
  - install/run/remove concepts
  - “store-only zip” packaging tools (`tools/pack_app.py`)
  - dev app workflow under `dev/apps_dev` with shell execution via `run_dev`
- **Embedded MicroPython runtime**
  - MicroPython is integrated as a submodule/runtime
  - `basalt.*` APIs exist and are expanding
  - UI API now includes draw + touch primitives used by apps like `paint`
- **Configuration system (CLI + Web)**
  - **Boards + Drivers + Platforms** model
  - CLI wizard: `python tools/configure.py --wizard`
  - CLI wizard now starts board-first with taxonomy filters:
    - Manufacturer -> Architecture -> Family -> Processor/Silicon -> Board
  - Local web configurator: `python tools/basaltos_config_server.py`
  - website-style landing + profile chip + board inventory
  - dedicated **App Market page** for:
    - browsing starter apps + market apps
    - add-to-current-build selection (board/platform gated)
    - uploading local market app zip packages
    - downloading catalog app packages as zip
  - ESP32 web flasher cache-hardening to avoid stale artifact flashes
  - generated outputs:
    - `config/generated/basalt_config.h`
    - `config/generated/basalt.features.json`
    - `config/generated/sdkconfig.defaults`
  - display boot splash options in driver config:
    - `none` (default)
    - `test_glyph` (built-in Basalt test glyph/pattern)
    - `local_file_upload` (currently expects processed `custom_image.c`; PNG/BMP conversion is planned later)
- **Multi-target backend groundwork**
  - runtime-capable flow for ESP32/IDF boards
  - generated-firmware profile flow started for constrained targets (PIC/AVR direction)
- **Driver/peripheral expansion**
  - connectivity: `wifi`, `bluetooth`, `twai` (CAN)
  - transceiver + motor: `mcp2544fd`, `uln2003`, `l298n`
  - sensing/display gates: `display_ssd1306`, `rtc`, `imu`, `dht22`, `mic`, `ads1115`, `mcp23017`, `hp4067`, `tp4056`

This release focuses on **workflow quality + platform breadth**: cleaner configurator UX, board-first taxonomy across wizard/UI, gated app selection in App Market, expanded board/driver catalogs, and practical runtime UI APIs for device-side apps.

---

## Big idea: Configure once, build anywhere

BasaltOS is moving toward a model where you choose:

1. **Platform** (esp32, stm32, rp2040, …)
2. **Board variant** (specific devkit/module/display/etc.)
3. **Drivers / features** (spi, uart, tft, filesystem, …)

…and BasaltOS generates:

- `config/generated/basalt_config.h` (compile-time feature switches + board pins)
- `config/generated/sdkconfig.defaults` (ESP-IDF defaults, layered by platform/board)

This is the “backend” that later becomes a GUI (Arduino/CubeMX-style), but it’s already usable today via CLI.

---

## Getting started (ESP-IDF / ESP32)

Quick path: `docs/ESP32_FIRST_SUCCESS_10_MIN.md`

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
- board taxonomy filters + board selection (e.g. `cyd_3248s035r`)
- driver selection (e.g. `spi,tft,uart,fs_spiffs`)

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

Third-party app developers should start with:
- `docs/third-party-app-dev.md`
- `docs/api-basalt.md`

App flow in web configurator:
1. Pick platform + board in Configurator
2. Open **App Market** (top nav)
3. Add compatible starter/market apps to current build
4. Generate + Build, then flash

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
- `drivers` (shows enabled drivers and whether runtime support is implemented yet)

Experimental driver note:
- `display_ssd1306`, `rtc`, `imu`, `dht22`, and `mic` are now selectable in the configurator.
- `rtc` now has a basic runtime API in MicroPython: `basalt.rtc.available()` and `basalt.rtc.now()`.
- `display_ssd1306` now has a basic runtime API in MicroPython: `basalt.ssd1306` with draw primitives (`pixel/line/rect/circle/ellipse`) and `show()`.
- `imu`, `dht22`, and `mic` remain **configuration gates + diagnostics** while runtime drivers are implemented incrementally.

Wi-Fi shell examples:
- `wifi status`
- `wifi scan`
- `wifi connect MySSID MyPassphrase`
- `wifi reconnect`
- `wifi disconnect`

Bluetooth shell examples:
- `bluetooth status`
- `bluetooth on`
- `bluetooth scan 5`
- `bluetooth off`

App/runtime examples:
- `apps` (list installed apps)
- `run paint` (interactive paint prototype using `basalt.ui`)

Third-party app dev helpers:
- `python tools/new_app.py my_app --dest apps`
- `python tools/validate_app.py apps/my_app`
- `python tools/pack_app.py apps/my_app dist/my_app.zip`

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
- Harden driver gating + board capability constraints
- Complete board metadata (pins, flash/ram, LED mode/polarity, driver support)
- Expand App Market metadata/signing and moderation workflow

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

## Contracts

See `CONTRACTS.md` and `docs/RELEASE_SYNC_CHECKLIST.md` for cross-repo ownership and release sync rules.
