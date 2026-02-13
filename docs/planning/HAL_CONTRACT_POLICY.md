# HAL Contract Policy v1

This policy defines HAL adapter status taxonomy and how partial coverage is managed.

## Status Taxonomy

- `complete`: all tracked HAL primitives are present for the adapter.
- `partial`: one or more tracked HAL primitives are missing and must be explicitly tracked.
- `missing`: no tracked HAL primitives are present for the adapter.

## No-Silent-Fallback Rule

- Any adapter with status other than `complete` must be explicitly acknowledged in policy metadata.
- Unacknowledged `partial` or `missing` adapter status is treated as a contract failure.

## Current Acknowledged Exceptions

- `partial`: `esp32`
- `missing`: none

Machine-readable policy source:

- `docs/planning/HAL_CONTRACT_POLICY.json`
