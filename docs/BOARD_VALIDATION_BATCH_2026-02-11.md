# Board Validation Batch (SCRUM-99)

Date: `2026-02-11`

## Target Scope

- ESP32-C6 DevKit (in-hand)
- M5StickC Plus2 (in-hand)
- AVR board in-hand
- PIC16F13145 Curiosity Nano

## Validation Performed (Configurator/Generation)

All commands run from `BasaltOS_Main/`.

### 1) ESP32 reference (control): C3 SuperMini

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers uart,gpio,shell_full \
  --outdir tmp/board_batch/esp32_c3_supermini_ref
```

Status: `PASS`

### 2) M5Stack Core2 (future profile prep, not in-hand)

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board m5stack_core2 \
  --enable-drivers uart,shell_full,tft,spi,i2c,fs_spiffs \
  --outdir tmp/board_batch/m5stack_core2
```

Status: `PROFILE PASS` (baseline profile added; no in-hand hardware validation)

### 3) M5StickC Plus2 (additional ESP32 display-family check)

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board m5stickc_plus2 \
  --enable-drivers uart,shell_full,tft,spi,i2c,fs_spiffs \
  --outdir tmp/board_batch/m5stickc_plus2
```

Status: `PASS` (after board metadata fix adding `uart_tx`/`uart_rx` aliases)

### 4) AVR Pro Micro (5V USB-C)

```bash
python3 tools/configure.py \
  --platform avr \
  --board arduino_pro_micro_5v_usbc \
  --enable-drivers uart,gpio,shell_full \
  --outdir tmp/board_batch/avr_pro_micro
```

Status: `PASS`

### 5) PIC16F13145 Curiosity Nano

```bash
python3 tools/configure.py \
  --platform pic16 \
  --board curiosity_nano_pic16f13145 \
  --enable-drivers uart,gpio,shell_full \
  --outdir tmp/board_batch/pic16f13145
```

Status: `PASS`

### 6) ESP32-C6

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c6 \
  --enable-drivers uart,gpio,shell_full \
  --outdir tmp/board_batch/esp32_c6
```

Status: `PASS` (initially blocked before baseline profile was added)

Status history:

- Initially blocked: no `esp32-c6` board profile.
- Resolved in this batch by adding baseline profile:
  - `boards/esp32/esp32-c6/board.json`

Validation command:

```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c6 \
  --enable-drivers uart,gpio,shell_full \
  --outdir tmp/board_batch/esp32_c6_basic2
```

Final status: `PASS`

## Outcome

- In-hand board batch execution completed for configurator/generation checks.
- ESP32-C6 is now represented by a baseline board profile and passes generation.
- M5StickC Plus2 configurator path is unblocked by pin alias fix.
- M5Stack Core2 profile exists for future use but is not in-hand validated in this batch.

## PIC16 Repeatable Bench Command

For repeat runs on PIC16F13145 Curiosity Nano (compile + drag-drop flash):

```bash
bash tools/pic16_curiosity_nano_run.sh
```
