# ADS1115 Hardware Verification Runbook

Purpose: verify BasaltOS `ads1115` shell diagnostics and a real ADC sanity read on hardware.

Scope: bring-up checks only (I2C probe + channel read path). This is not a full sensor-calibration workflow.

## 1) Prerequisites

- Board supported by BasaltOS (`esp32` platform).
- ADS1115 module wired to board I2C pins.
- Firmware built with:
  - `ads1115`
  - `i2c`
  - `gpio`

## 2) Wiring Checklist

Use your board profile as source of truth.

Required Basalt pins:
- `BASALT_PIN_I2C_SDA`
- `BASALT_PIN_I2C_SCL`

Power and signal sanity:
- ADS1115 `VDD` and `GND` connected correctly.
- `ADDR` pin wired for expected address (`0x48` default).
- Shared ground between MCU and ADS1115 module.
- Start with `AIN0` tied to GND for first sanity read.

## 3) Configure + Build

Example configure flow:

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board <your-board> \
  --enable-drivers ads1115,i2c,gpio,shell_full \
  --outdir config/generated
```

Optional config check:

```bash
rg "BASALT_ENABLE_ADS1115|BASALT_ENABLE_I2C|BASALT_PIN_I2C_|BASALT_CFG_ADS1115" config/generated/basalt_config.h
```

## 4) Shell Validation Sequence

At `basalt>` prompt:

```text
drivers
ads1115 status
ads1115 probe
ads1115 read
ads1115 read 0
ads1115 read 1
```

Optional channel sanity:
- Tie `AIN0` to GND and run `ads1115 read 0` (expect raw/mV near 0).
- Tie `AIN0` to a known positive voltage and rerun `ads1115 read 0`.
- If using default `pga_mv=2048`, inputs above about 2.048V will clip near full scale.

## 5) Expected Output Patterns

`drivers`:
- `ads1115: enabled (shell API: ads1115 status/probe/read)`

`ads1115 status` should show:
- `ads1115.enabled: yes`
- configured address/mode/PGA/SPS
- bound I2C pins (not `-1`)
- `ads1115.probe: ok`

`ads1115 probe`:
- same probe path as `status`, should report `ads1115.probe: ok`

`ads1115 read [ch]`:
- `ads1115 read: ch=<0..3> raw=<signed> mv=<signed>`

## 6) Pass/Fail Criteria

Pass:
- `status`/`probe` succeed without I2C errors.
- `read` returns stable numeric values.
- Grounded input reads close to 0mV.

Fail:
- any `ads1115 ...: fail (...)` output.
- I2C pin config invalid or unbound.
- channel reads always saturate or are clearly non-physical for known wiring.

## 7) Triage Guide

- `enable ads1115+i2c drivers`:
  - include both `ads1115` and `i2c` in driver set.
- `invalid I2C pins`:
  - fix board pin bindings and regenerate config.
- `i2c_driver_install failed` or register read/write failures:
  - check pull-ups, address selection, power, and shared ground.

## 8) Evidence to Capture in Jira

Paste into Jira comment:
- board name + ADS1115 module variant
- wiring summary (SDA/SCL/address and tested channel wiring)
- configure command used
- shell outputs for validation sequence
- pass/fail result and follow-up issue key (if needed)
