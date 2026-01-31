# BasaltOS

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
