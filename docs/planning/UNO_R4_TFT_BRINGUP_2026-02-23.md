# UNO R4 WiFi + 3.5" TFT Shield Bring-up (2026-02-23)

## Hardware observed
- Board: Arduino Uno R4 WiFi (`arduino:renesas_uno:unor4wifi`)
- Shield: red PCB, silkscreen `3.5" TFT LCD for arduino uno`
- Touch: resistive
- Interface: 8-bit parallel (LCD_D0..D7 + LCD_RD/WR/RS/CS/RST)

## Bench result
- Upload + runtime serial probe: `LCD ID = 0x6814`
- Probe sketch path:
  - `tools/bench/uno_r4_tft_probe/uno_r4_tft_probe.ino`
- Interactive shell+paint+calibration+flappy bench sketch path:
  - `tools/bench/uno_r4_tft_shell_paint/uno_r4_tft_shell_paint.ino`

## Important library note
Current upstream `MCUFRIEND_kbv` release does not support UNO R4 by default.
Local patch applied on this machine:
- `/home/homer/Arduino/Projects/libraries/MCUFRIEND_kbv/utility/mcufriend_shield.h`
- Added fallback backend for:
  - `ARDUINO_UNOR4_WIFI`
  - `ARDUINO_UNOR4_MINIMA`

## Use
Compile/upload with:
```bash
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_probe
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_probe
```

Shell+paint bench:
```bash
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_shell_paint
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_shell_paint
```

Useful shell commands in bench app:
- `help`
- `paint on` / `paint off`
- `color white|red|green|blue|yellow`
- `touchwatch on` (raw touch stream)
- `cal start` (4-corner touch calibration), `cal show`, `cal reset`
- `flappy on` / `flappy off` (touch-to-flap mini demo)
