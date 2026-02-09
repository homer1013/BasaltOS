# Hardware-First Prioritized Backlog (2026-02-09)

Goal: maximize completed, hardware-validated value using hardware currently on hand.

## Priority 1 (Immediate, fully testable now)

1. TP4056 on ESP32-C3 SuperMini
- Validate `tp4056 status/on/off` behavior on real module.
- Capture pin mapping + expected shell output artifact.

2. SSD1306 OLED bring-up (128x64 + 128x32)
- Verify display init paths across both form factors.
- Confirm config profile differences required (if any).

3. L298N + ULN2003 runtime verification
- Validate motor driver shell commands on hardware.
- Record known-good pin mappings and safe test procedure.

4. ADS1115 runtime diagnostics
- Add/verify shell diagnostics and real ADC read sanity check.

5. MCP23017 runtime diagnostics
- Add/verify shell diagnostics for register read/write and pin mode toggles.

## Priority 2 (High value, mostly software + optional hardware validation)

6. DHT22 runtime shell path verification
- Validate probe/read behavior with available DHT22 module.

7. DHT11 support decision
- Either add `dht11` module/runtime path or define compatibility wrapper policy.

8. Board validation pass for in-hand boards
- ESP32-C6 DevKit
- M5Stack Core2
- Arduino Uno R3 / Mega 2560 / Uno R4 WiFi
- Pro Micro USB-C
- PIC16F13145 Curiosity Nano

## Priority 3 (Deferred by missing hardware)

9. MCP2515 final hardware verification
- Keep software complete; run full hardware pass once module arrives.

10. BME280 final hardware verification
- Keep software complete; run sensor-read validation once module arrives.

11. ESP32-S3 DevKitC hardware verification
- Existing software profile/smoke in place; execute when board arrives.

## Execution Rule

- If module/board is in-hand: task must include hardware output evidence before done.
- If not in-hand: task can close as software-ready only with a clear hardware-pending note.
