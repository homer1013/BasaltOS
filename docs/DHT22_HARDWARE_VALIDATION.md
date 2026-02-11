# DHT22 Hardware Validation (SCRUM-97)

Target board: `esp32-c3-supermini`  
Date: `2026-02-11`

## Known-Good Wiring

- `DHT22 +` -> `3V3`
- `DHT22 -` -> `GND`
- `DHT22 OUT` -> `GPIO10`
- External pull-up: `10k` resistor from `OUT` to `3V3`

## Firmware Configuration Used

`config/generated/basalt_config.h`:

- `#define BASALT_ENABLE_DHT22 1`
- `#define BASALT_PIN_DHT22_DATA 10`
- `#define BASALT_CFG_DHT22_SAMPLE_MS 2000`

## Build/Flash

```bash
source tools/env.sh
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build
idf.py -B build -p /dev/ttyACM0 flash monitor
```

## Shell Validation Commands

```text
dht22 status
dht22 read 10
dht22 read 10
```

## Observed Pass Output

```text
dht22 read 10
dht22 read: ok
dht22.pin_used: 10
dht22.temp_c: 23.5
dht22.humidity_pct: 49.1
dht22.raw: 01 EB 00 EB D7

dht22 read 10
dht22 read: ok
dht22.pin_used: 10
dht22.temp_c: 22.8
dht22.humidity_pct: 49.6
dht22.raw: 01 F0 00 E4 D5
```

## Notes

- Runtime decode was aligned to the Adafruit/Arduino pulse strategy (`high pulse > low pulse` bit decision), which resolved bench timeouts.
- Use `dht22 read [pin]` during bring-up to test alternate GPIO mappings without reflashing.
