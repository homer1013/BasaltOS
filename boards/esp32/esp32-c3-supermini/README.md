ESP32-C3 SuperMini baseline profile.

This profile now includes a software-first MCP2515/BME280 bring-up template:
- SPI: `spi_mosi=6`, `spi_miso=7`, `spi_sclk=10`
- MCP2515: `can_cs=8`, `can_int=1`
- BME280: default I2C address `0x76`

Important:
- C3 SuperMini clone pinouts vary a lot by vendor revision.
- Treat the mapping above as a starting template only.
- Hardware validation is required when modules are available.

Suggested validation sequence (once hardware is available):
- `drivers`
- `mcp2515 status`
- `mcp2515 probe`
- `mcp2515 tx 0x123 A1B2`
- `mcp2515 rx`
- `bme280 status`
- `bme280 read`
