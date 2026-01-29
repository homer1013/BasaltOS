# Basalt OS board selection

Basalt uses a simple board profile system to switch targets quickly.
Each profile provides `sdkconfig.defaults` and `partitions.csv`.

## Current board (connected)
- ESP32 CYD (ESP32-D0WD-V3)
- Serial: /dev/ttyUSB0

## Switch boards
```
./tools/board.sh esp32-cyd
```

Then build/flash with ESP-IDF:
```
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Notes
- Profiles for ESP32-C3/C6 are placeholders; they need proper partitions and target selection.
- ESP32-C3/C6 require `idf.py set-target esp32c3` or `idf.py set-target esp32c6`.
- CYD display support is currently experimental and assumes an SPI ILI9488 panel; see `main/tft_console.c` for pin config.
