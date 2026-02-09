# SSD1306 Hardware Validation (SCRUM-93)

## Target Bench
- Controller: ESP32-C3 SuperMini
- Displays: SSD1306 `128x64` and `128x32`
- Transport: I2C

## Wiring (ESP32-C3 SuperMini default profile)
- `SDA` -> GPIO4
- `SCL` -> GPIO3
- `VCC` -> 3V3
- `GND` -> GND

Most modules use address `0x3C`; some are `0x3D`.

## Firmware Prep (common)
From repo root:

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers i2c,display_ssd1306,fs_spiffs,shell_full
```

## Pass A: 128x64 panel
Build/flash with default geometry (128x64 fallback):

```bash
idf.py -B build flash monitor
```

In shell:

```text
drivers
apps
run ssd1306_shapes
logs
```

Expected serial evidence lines:
- `ssd1306_shapes: ready=1 width=128 height=64`
- `ssd1306_shapes: rendered`

## Pass B: 128x32 panel
Set display height override to 32 for this pass.

```bash
# apply one-line override (re-run after each configure.py call)
echo '#define BASALT_CFG_DISPLAY_SSD1306_HEIGHT 32' >> config/generated/basalt_config.h
idf.py -B build flash monitor
```

In shell:

```text
run ssd1306_shapes
logs
```

Expected serial evidence lines:
- `ssd1306_shapes: ready=1 width=128 height=32`
- `ssd1306_shapes: rendered`

## If Display Is Not Detected
- Swap address by adding this line to `config/generated/basalt_config.h` and rebuild:

```c
#define BASALT_CFG_DISPLAY_SSD1306_ADDRESS "0x3D"
```

- Re-check wiring and common ground.
- Keep cable length short.

## Jira Evidence Checklist
For SCRUM-93 comment:
- Panel variant tested (`128x64` / `128x32`)
- Address used (`0x3C` or `0x3D`)
- Output lines from `ssd1306_shapes` run
- Notes on any rendering quirks (clipping, inversion, flicker)

## Done Criteria
- Both panel variants render shapes/text on bench.
- Geometry lines confirm correct height in each pass.
- Known-good wiring + address documented in SCRUM-93 comment.
