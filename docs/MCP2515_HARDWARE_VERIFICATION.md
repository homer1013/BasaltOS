# MCP2515 Hardware Verification Runbook

Purpose: verify the BasaltOS `mcp2515` shell diagnostics on real hardware.

Scope: bring-up checks only (power/pins/SPI/register/TX-RX visibility). This is not full CAN network conformance testing.

## 1) Prerequisites

- Board supported by BasaltOS (`esp32` platform).
- MCP2515 module wired to board SPI and interrupt pins.
- Optional MCP2551/TJA1050 transceiver and second CAN node for meaningful RX/TX bus behavior.
- Firmware built with these drivers enabled:
  - `mcp2515`
  - `spi`
  - `gpio`

## 2) Wiring Checklist

Use your board profile pin map as source of truth.

Required Basalt pins:
- `BASALT_PIN_SPI_SCLK`
- `BASALT_PIN_SPI_MISO`
- `BASALT_PIN_SPI_MOSI`
- `BASALT_PIN_CAN_CS`
- `BASALT_PIN_CAN_INT`

Power and bus sanity:
- MCP2515 module VCC/GND correct for module variant.
- CANH/CANL only connected to compatible transceiver/network.
- Shared ground between MCU and CAN hardware.

## 3) Configure + Build

Example configure flow:

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board <your-board> \
  --enable-drivers mcp2515,spi,gpio \
  --outdir config/generated
```

Optional quick config check:

```bash
rg "BASALT_ENABLE_MCP2515|BASALT_ENABLE_SPI|BASALT_PIN_CAN_CS|BASALT_PIN_CAN_INT|BASALT_PIN_SPI_" config/generated/basalt_config.h
```

## 4) Shell Bring-up Sequence

Run in Basalt shell:

```text
drivers
mcp2515 status
mcp2515 probe
mcp2515 reset
mcp2515 read 0x0E
mcp2515 write 0x36 0x5A
mcp2515 read 0x36
mcp2515 tx 0x123 A1B2C3D4
mcp2515 rx
```

## 5) Expected Output Patterns

`drivers`:
- `mcp2515: enabled (shell API: mcp2515 status/probe/reset/read/write/tx/rx)`

`mcp2515 status` should show:
- `mcp2515.enabled: yes`
- `mcp2515.spi_required: yes`
- bound SPI/CAN pins (not `-1`)

`mcp2515 probe` success pattern:
- `mcp2515 probe: ok (CANSTAT=0x.. TXB0D0 rw=ok)`

`mcp2515 reset`:
- `mcp2515 reset: ok`

`mcp2515 read`:
- `mcp2515 read: reg=0x.. val=0x..`

`mcp2515 write`:
- `mcp2515 write: reg=0x.. val=0x.. verify=0x..`

`mcp2515 tx`:
- `mcp2515 tx: id=0x... dlc=.. txb0ctrl=0x..`

`mcp2515 rx`:
- always prints `rx_status` + `canintf`
- if RX0 has data, also prints `mcp2515 rx0: dlc=.. data=...`

## 6) Pass/Fail Criteria

Pass (bring-up complete):
- `status` reports expected enabled gates and real pin values.
- `probe` returns `ok`.
- `read/write` sequence succeeds with verify match.
- `tx` executes without SPI/register errors.
- `rx` returns status fields without SPI/register errors.

Fail (needs follow-up):
- any `mcp2515 ...: fail (...)` output.
- pin values show `-1` when feature is enabled.
- write verify mismatch.

## 7) Triage Guide

- `spi driver not enabled`:
  - ensure `spi` is included in `--enable-drivers`.
- `missing MCP2515 pins` or `missing SPI pins`:
  - fix board metadata pin bindings and regenerate config.
- `spi bus init failed` / `spi add device failed`:
  - pin conflict or host already configured incorrectly; inspect other SPI consumers.
- repeated RX empty but TX appears fine:
  - likely no valid CAN peer/termination/bitrate alignment on bus.

## 8) Evidence to Capture in Jira

When hardware testing is run, paste into Jira comment:
- board name and wiring summary
- configure command used
- exact shell outputs for the sequence above
- pass/fail result
- follow-up issue key (if failures)

## 9) Notes

- Current commands are diagnostics-oriented and intentionally minimal.
- Full CAN workflow validation (multi-node traffic reliability, bitrate matrix, error-frame behavior) should be tracked as a follow-up task.
