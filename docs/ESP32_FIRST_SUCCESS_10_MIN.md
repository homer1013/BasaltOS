# ESP32 First Success in 10 Minutes

This is the fastest validated path from clone to first BasaltOS build/flash on one reference board.

Reference board in this guide:
- `esp32-c3-supermini`

## Timebox
- 1 min: clone + submodules
- 1 min: load toolchain env
- 2 min: generate config
- 4 min: build
- 2 min: flash + monitor sanity check

## Prerequisites
- Linux/macOS shell
- ESP-IDF installed and working
- USB serial access to your board (`/dev/ttyUSB0` or similar)

## 1) Clone and initialize
```bash
git clone https://github.com/homer1013/BasaltOS
cd BasaltOS
git submodule update --init --recursive
```

Expected:
- repo checkout completes
- submodules initialize without errors

## 2) Load BasaltOS environment
```bash
source tools/env.sh
```

Expected:
- no shell errors
- `idf.py --version` works

## 3) Generate board config (non-interactive)
```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uart \
  --outdir config/generated
```

Expected generated files:
- `config/generated/basalt_config.h`
- `config/generated/sdkconfig.defaults`
- `config/generated/basalt.features.json`

## 4) Build firmware
```bash
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build set-target esp32c3
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build
```

Expected:
- build exits `0`
- firmware artifacts exist under `build/`

## 5) Flash and monitor
```bash
idf.py -B build -p /dev/ttyUSB0 flash monitor
```

Expected runtime sanity checks:
- device boots without panic/reset loop
- shell prompt appears
- `help -commands` prints command list

Exit monitor:
- `Ctrl+]`

## If your serial port differs
List likely serial devices:
```bash
ls -1 /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```
Then replace `/dev/ttyUSB0` with the correct port.

## Definition of first success
You are successful when all are true:
- config generation succeeded
- firmware built successfully
- flash completed
- shell is reachable and responds to `help -commands`

## Next steps after first success
- Try local configurator: `python tools/basaltos_config_server.py`
- Read API docs: `docs/api-basalt.md`
- Build a starter app: `docs/third-party-app-dev.md`
