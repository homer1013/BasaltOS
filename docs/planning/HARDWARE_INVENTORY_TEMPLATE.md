# Hardware Inventory Template

Fill this once, then we prioritize board/driver work only around hardware-in-hand.

## Boards You Physically Have

| Board | Qty | MCU/SoC | Notes |
| --- | ---: | --- | --- |
| example: ESP32-C3 SuperMini | 2 | ESP32-C3 | vendor clone; unknown LED pin |

## IC/Module Inventory (Drivers)

| IC/Module | Qty | Interface | Target Driver | Notes |
| --- | ---: | --- | --- | --- |
| example: MCP2515 module | 0 | SPI | mcp2515 | not yet in hand |
| example: BME280 | 0 | I2C | bme280 | not yet in hand |

## Dev/Debug Accessories

| Item | Qty | Notes |
| --- | ---: | --- |
| USB-UART adapter |  |  |
| Logic analyzer |  |  |
| Breadboard + jumpers |  |  |
| CAN transceiver module |  |  |

## Priority Preferences

- Preferred immediate focus (pick one):
  - board bring-up
  - sensor drivers
  - comms drivers
  - shell/runtime reliability
- Timebox preference:
  - fast wins (<2h tasks)
  - medium (half-day)
  - deep work (1-2 day)

## Hardware-Gated Backlog Rule

- If hardware qty is `0`, task is marked **software-complete / hardware-pending**.
- If hardware qty is `>=1`, task can be planned for full validation and closure.
