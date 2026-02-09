# TP4056 Hardware Validation (SCRUM-92)

## Goal
Validate `tp4056 status/on/off` behavior on real hardware (ESP32-C3 SuperMini + TP4056 module), and record known-good wiring + expected shell output.

## Preconditions
- Board: ESP32-C3 SuperMini
- Module: TP4056 (with exposed CHRG/DONE; CE optional)
- Build includes drivers: `gpio,tp4056`
- Shell available over serial

## Wiring
Pick free GPIO pins and keep this mapping in your session notes.

Example mapping:
- `tp4056_chrg` -> GPIO4
- `tp4056_done` -> GPIO5
- `tp4056_ce`   -> GPIO6 (optional)
- GND shared between ESP32-C3 and TP4056 module

Notes:
- TP4056 CHRG/DONE are typically active-low open-drain style outputs.
- Default module option `status_pins_active_low=true` should usually be correct.
- If CE is not wired, set `ce_pin_present=false`.

## Configure
From repo root, generate config with TP4056 enabled and your selected pins.

```bash
python3 configure.py \
  --board esp32-c3-supermini \
  --modules gpio,tp4056 \
  --set-pin tp4056_chrg=4 \
  --set-pin tp4056_done=5 \
  --set-pin tp4056_ce=6 \
  --set-option tp4056.status_pins_active_low=true \
  --set-option tp4056.ce_pin_present=true \
  --set-option tp4056.ce_active_high=true
```

If CE is not connected, remove `--set-pin tp4056_ce=...` and set `tp4056.ce_pin_present=false`.

## Build/Flash
```bash
idf.py build flash monitor
```

## Shell Validation Sequence
Run these in Basalt shell:

```text
drivers
tp4056 status
tp4056 on
tp4056 status
tp4056 off
tp4056 status
```

Expected output keys to capture:
- `tp4056.enabled: yes`
- `tp4056.pins: chrg=... done=... ce=...`
- `tp4056.charging: yes|no|n/a`
- `tp4056.done: yes|no|n/a`
- `tp4056.state: charging|done|idle|unknown`
- `tp4056.ce_pin_present: yes|no`
- `tp4056.ce_enabled: yes|no` (if CE present)

## Evidence to Attach (Jira Comment)
- Exact pin map used
- Module options used (`status_pins_active_low`, `ce_pin_present`, `ce_active_high`)
- One captured output block for each state observed
- Any mismatch between LED indicators and shell state

## Completion Criteria
- `tp4056 status` returns valid structured fields without errors
- If CE wired: `tp4056 on/off` toggles `tp4056.ce_enabled` as expected
- Known-good mapping and caveats documented in SCRUM-92 comment
- If hardware behavior differs by TP4056 breakout variant, note variant-specific caveat
