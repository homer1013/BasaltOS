# BasaltOS v0.1.0 Acceptance Checklist

This checklist is the acceptance gate for Sprint 1 (`SCRUM-19`, `SCRUM-20`) and the baseline reference for `v0.1.0`.

## Scope

Pass/fail is based on:

1. CLI configuration reliability
2. Local configurator reliability
3. Metadata validation health
4. End-to-end smoke workflow and artifact consistency

## Evidence Capture

Store run evidence here:

- `tmp/acceptance/v0.1.0/<date>/commands.log`
- `tmp/acceptance/v0.1.0/<date>/artifacts/`
- `tmp/acceptance/v0.1.0/<date>/notes.md`

Example setup:

```bash
DATE=$(date +%F)
OUT="tmp/acceptance/v0.1.0/$DATE"
mkdir -p "$OUT/artifacts"
exec > >(tee -a "$OUT/commands.log") 2>&1
```

## Gate A: CLI Configuration

### A1. Board discovery works

```bash
python3 tools/configure.py --list-boards
```

Pass criteria:

- Command exits `0`
- Lists boards from multiple platforms

### A2. Driver discovery works

```bash
python3 tools/configure.py --list-drivers
```

Pass criteria:

- Command exits `0`
- Driver IDs list is non-empty

### A3. Non-interactive generation works for reference boards

Run for one board per family (ESP32, STM32, RP2040, AVR) using real board IDs from `--list-boards`:

```bash
python3 tools/configure.py --platform esp32 --board esp32-c3-supermini --enable-drivers uart --outdir tmp/acceptance/v0.1.0/$DATE/artifacts/esp32
python3 tools/configure.py --platform stm32 --board nucleo_f401re --enable-drivers uart --outdir tmp/acceptance/v0.1.0/$DATE/artifacts/stm32
python3 tools/configure.py --platform rp2040 --board raspberry_pi_pico --enable-drivers uart --outdir tmp/acceptance/v0.1.0/$DATE/artifacts/rp2040
python3 tools/configure.py --platform atmega --board arduino_uno_r3 --enable-drivers uart --outdir tmp/acceptance/v0.1.0/$DATE/artifacts/avr
```

Pass criteria:

- Each command exits `0`
- Output dir contains generated config artifacts
- No silent failures

### A4. CLI failure path is actionable

```bash
python3 tools/configure.py --platform esp32 --board does-not-exist --enable-drivers uart
```

Pass criteria:

- Command exits non-zero
- Error message clearly indicates invalid board input

## Gate B: Local Configurator

### B1. Server starts

```bash
python3 tools/basaltos_config_server.py
```

Pass criteria:

- Server starts without traceback
- UI reachable at `http://127.0.0.1:5000`

### B2. API endpoints return valid JSON

In another terminal while server is running:

```bash
curl --fail-with-body -sS http://127.0.0.1:5000/api/platforms > tmp/acceptance/v0.1.0/$DATE/artifacts/platforms.json
curl --fail-with-body -sS http://127.0.0.1:5000/api/boards/esp32 > tmp/acceptance/v0.1.0/$DATE/artifacts/boards_esp32.json
curl --fail-with-body -sS "http://127.0.0.1:5000/api/drivers?platform=esp32" > tmp/acceptance/v0.1.0/$DATE/artifacts/drivers_esp32.json
```

Pass criteria:

- All commands exit `0`
- Output JSON files are non-empty

### B3. UI smoke checks pass

Optional automated checks (recommended when Playwright is installed):

```bash
node tools/e2e/smoke_configurator.js
```

Pass criteria:

- Smoke test exits `0`
- No blocking UI regression reported

## Gate C: Metadata Validation

### C1. Metadata validation passes

```bash
python3 tools/validate_metadata.py
```

Pass criteria:

- Command exits `0`
- Reports validated boards/modules with no fatal errors

## Gate D: Build and Runtime Smoke

### D1. Build completes

```bash
bash tools/build.sh
```

Pass criteria:

- Build exits `0`
- No fatal compile/link errors

### D2. Flash + monitor workflow executes (hardware-attached)

```bash
bash tools/flash.sh /dev/ttyUSB0
bash tools/monitor.sh /dev/ttyUSB0
```

Pass criteria:

- Flash exits `0`
- Monitor opens serial output and runtime starts

## Release Decision Matrix

Mark each gate:

- `PASS`
- `PASS WITH NOTES`
- `FAIL`

`v0.1.0` baseline is ready only when all mandatory gates are `PASS` or `PASS WITH NOTES` and all `FAIL` items have linked Jira bugs.

## Jira Mapping

- `SCRUM-19`: Draft checklist skeleton
- `SCRUM-20`: Add exact command set + expected outputs
- `SCRUM-23`..`SCRUM-39`: Execution evidence and findings
- `SCRUM-16`: Consolidated top-10 gaps after runs
