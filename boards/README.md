# Board profiles

Board profiles provide a simple "BIOS-like" switch for Basalt OS by copying
board-specific `sdkconfig.defaults` and `partitions.csv` into the repo root.

Current profiles:
- esp32-cyd (ESP32-D0WD-V3) [active target]
- esp32-wroom-32d (placeholder)
- esp32-cam (placeholder)
- esp32-c3 (placeholder)
- esp32-c6 (placeholder)

Use `tools/board.sh <board-name>` to activate a profile.

