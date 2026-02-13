# HAL Runtime Contract Policy v1

This policy controls runtime init/status contract coverage checks for required HAL ports.

## Tracked Primitives

- `gpio`
- `i2c`
- `uart`

## Rule

- Required ports must have runtime function bodies for tracked primitives.
- If not, the port must be explicitly acknowledged in policy metadata.
- Unacknowledged runtime gaps fail the smoke gate.

## Current Acknowledged Missing Runtime Implementation Ports

- `esp32c3`
- `esp32c6`
- `esp32s3`
- `pic16`
- `ra4m1`
- `rp2040`
- `stm32`

Machine-readable policy source:

- `docs/planning/HAL_RUNTIME_CONTRACT_POLICY.json`
