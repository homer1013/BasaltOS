# L298N + ULN2003 Hardware Validation (SCRUM-94)

## Goal
Validate BasaltOS shell/runtime behavior for:
- `l298n` (dual H-bridge command path)
- `uln2003` (stepper/coil command path)

Target bench controller:
- ESP32-C3 SuperMini

## Safety
- Use an external motor supply for L298N/ULN2003 loads.
- Tie grounds together: ESP32-C3 GND, module GND, motor supply GND.
- Start with no motor/load connected for command-path sanity checks.
- Only connect motors after `status`/`stop`/`off` commands behave correctly.

## Build/flash baseline
From repo root:

```bash
source tools/env.sh
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers gpio,uart,shell_full,l298n,uln2003
```

Then set runtime pin macros in `config/generated/basalt_config.h` (examples below), build, and flash:

```bash
idf.py -B build build
idf.py -B build -p /dev/ttyACM0 flash monitor
```

## Pin mapping examples (bench-safe defaults)
Use one module at a time for clean validation.

### ULN2003-only mapping example
Set in `config/generated/basalt_config.h`:
- `BASALT_PIN_ULN2003_IN1 2`
- `BASALT_PIN_ULN2003_IN2 3`
- `BASALT_PIN_ULN2003_IN3 4`
- `BASALT_PIN_ULN2003_IN4 5`

Optional config:
- `BASALT_CFG_ULN2003_ACTIVE_HIGH 1`
- `BASALT_CFG_ULN2003_STEP_DELAY_MS 10`

### L298N-only mapping example
Set in `config/generated/basalt_config.h`:
- `BASALT_PIN_L298N_IN1 2`
- `BASALT_PIN_L298N_IN2 3`
- `BASALT_PIN_L298N_ENA 4`
- `BASALT_PIN_L298N_IN3 5`
- `BASALT_PIN_L298N_IN4 20`
- `BASALT_PIN_L298N_ENB 21`

Optional config:
- `BASALT_CFG_L298N_IN_ACTIVE_HIGH 1`
- `BASALT_CFG_L298N_EN_ACTIVE_HIGH 1`

## Shell validation sequence
At `basalt>` prompt:

### ULN2003
```text
uln2003 status
uln2003 test
uln2003 step 16 10
uln2003 step -16 10
uln2003 off
```

Expected:
- `status` prints enabled + pin map + config values.
- `test` should cycle IN1..IN4 with visible ULN board LEDs.
- `step` completes and returns `uln2003 step: done (...)`.
- Bench note (ESP32-C3 SuperMini + 28BYJ-48): use >=10ms delay for reliable rotation.
- `off` returns `uln2003: coils off`.

### L298N
```text
l298n status
l298n test
l298n a fwd
l298n a rev
l298n a stop
l298n b fwd
l298n b rev
l298n b stop
l298n stop
```

Expected:
- `status` prints enabled + pin map + active-high config.
- `test` returns `l298n test: ...` and then `l298n test: done`.
- channel commands return `l298n: channel a|b -> ...`.
- `stop` returns `l298n: both channels stopped`.
- Bench note: if motor starts at power-up before firmware commands, L298N inputs are likely floating; power motor rail after MCU init or add pull-down resistors on IN pins.
- Bench note: if direction feels inverted, swap motor leads on the affected OUT pair.

## Evidence to capture for Jira
- Board + module used (exact module variant if known).
- Final pin map used.
- Full monitor log section for command sequence.
- Any mismatch between expected/actual behavior.
- Follow-up bug list (if any), each with repro steps.

## Pass criteria
- Both command surfaces are interactive and deterministic.
- No runtime errors/panics during command sequence.
- Motor/coil output behavior matches command intent.
- Known-good wiring and command recipe documented.
