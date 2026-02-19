# BasaltOS Stable API Surface v1 (Foundation)

This document defines the small API surface that should remain stable for early app developers.

## Scope

Stable foundation API groups:

- `system`
- `fs`
- `gpio`
- `i2c`
- `ui`
- `net`

## Contract Intent

- Keep names and high-level semantics stable.
- Add new capabilities by extension, not by breaking existing behavior.
- Mark non-stable areas as `experimental` in docs and release notes.

## Runtime Readiness Categories

- `ready`: normal runtime use expected for supported boards.
- `partial`: works on some boards/paths; may need setup or constraints.
- `config-only`: selection/diagnostics exist, runtime implementation incomplete.

CLI visibility command:

```bash
python tools/configure.py --list-runtime-status
```

## Current v1 Baseline

- `system`: logging + lifecycle status surfaces are active.
- `fs`: SPIFFS path is stable baseline; SD is board/config dependent.
- `gpio`: stable core read/write/mode path.
- `i2c`: stable scan/transport baseline.
- `ui`: active in MicroPython path; broader parity remains in progress.
- `net`: baseline shell/runtime flows exist; hardware/network-dependent.

## Compatibility Rule

For v1-stable groups, avoid breaking changes to identifier names and top-level behavior without a migration window and release-note callout.
