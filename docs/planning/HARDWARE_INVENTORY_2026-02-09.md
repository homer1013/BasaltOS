# Hardware Inventory (2026-02-09)

Source: in-hand hardware snapshot provided by Homer.

## Boards

| Board | Qty | Notes |
| --- | ---: | --- |
| Arduino Uno R3 | 1 | atmega328 |
| Arduino Mega 2560 | 1 | ATmega2560 |
| Arduino Uno R4 WiFi | 1 | Renesas RA4M1 |
| ESP32-C3 SuperMini | 5 | primary high-throughput dev board |
| ESP32 CYD (ESP32-3248S035R) | 1 | display-focused |
| M5StickC Plus2 | 1 | compact display-focused ESP32 board |
| ESP32-C6 DevKit | 1 | newer ESP32 target |
| Circuit Playground Express (ATSAMD21) | 1 | ATSAMD21 platform coverage |
| Raspberry Pi 5 | 1 | linux-sbc platform |
| Arduino Pro Micro (5V USB-C) | 1 | AVR board variant |
| ESP8266 NodeMCU | 1 | current repo support gap (future) |
| ESP32-CAM | 1 | board profile gap (future) |
| PIC16F13145 Curiosity Nano | 1 | PIC16 hardware coverage |

## Modules / ICs

| Module/IC | Qty | Mapped Driver/Area | Notes |
| --- | ---: | --- | --- |
| MCP23017 | 3 | `mcp23017` | I2C GPIO expander |
| TP4056 module | 3 | `tp4056` | battery charging status/control |
| L298N module | 1 | `l298n` | DC motor control |
| ULN2003AFG module | 1 | `uln2003` | stepper/coil driver |
| OLED 128x64 SSD1306 | 1 | `display_ssd1306` | display bring-up |
| OLED 128x32 SSD1306 | 1 | `display_ssd1306` | display bring-up variant |
| HP4067 module | 1 | `hp4067` | analog mux path |
| ADS1115 module | 1 | `ads1115` | external ADC |
| DHT11 module | 1 | DHT family | likely requires new `dht11` module/task |
| DHT22 module | 1 | `dht22` | environmental sensor |

## Optional Tools / Accessories

| Item | Qty | Notes |
| --- | ---: | --- |
| XGecu TL866 II Plus programmer | 1 | useful for MCU programming workflows |

## Constraints

- Not currently available in-hand: MCP2515, BME280, ESP32-S3-DevKitC-1, M5Stack Core2.
- Tasks requiring these should be marked **software-complete / hardware-pending** until hardware is available.
