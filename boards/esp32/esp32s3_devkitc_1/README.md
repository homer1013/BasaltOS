ESP32-S3-DevKitC-1 reference template profile.

This profile includes software-level defaults for:
- MCP2515 (`can_cs`, `can_int`, bitrate/oscillator)
- BME280 (`i2c_address`, sample rate)

Hardware verification is pending until modules/board are available.
When hardware arrives, use:
- `mcp2515 status|probe|tx|rx`
- `bme280 status|read`
