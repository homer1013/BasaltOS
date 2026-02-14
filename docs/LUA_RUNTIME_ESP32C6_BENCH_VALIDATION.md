# Lua Runtime ESP32-C6 Bench Validation

This document defines the first bench validation pass for Lua runtime pipeline checks on ESP32-C6.

## Scope

This pass verifies runtime plumbing, not Lua VM execution completeness:

- firmware boots with Lua runtime component enabled
- shell reports Lua runtime readiness status
- app launch path resolves `runtime = "lua"` and `.lua` entrypoints
- runtime diagnostics (`logs`) reflect `last_runtime`/`last_result`/`last_error`
- stop behavior is deterministic when no runtime task is active

## Prerequisites

- ESP32-C6 board connected by USB
- working ESP-IDF environment (`source tools/env.sh`)
- serial port available (default `/dev/ttyACM0`)

Optional overrides:

- `BASALT_PORT=/dev/ttyACM1`
- `BASALT_BUILD_DIR=build_c6_lua_bench`

## Scripted Smoke

Run:

```bash
bash tools/tests/esp32c6_lua_bench_smoke.sh
```

The script configures for ESP32-C6, builds with:

- `-D BASALT_ENABLE_LUA_RUNTIME=ON`

and executes shell commands:

- `logs`
- `run /apps/lua_hello`
- `run /apps/lua_blink`
- `logs`
- `stop`
- `logs`

Expected outcomes:

- launch lines indicate `(lua)` runtime selection
- runtime logs include `runtime.last_runtime: lua`
- current foundation state reports:
  - `lua vm execution is not integrated in this build yet`
- `stop` reports `no app running` after failed launch attempts

## Fixtures Used

SPIFFS apps used by this bench pass:

- `spiffs/apps/lua_hello`
- `spiffs/apps/lua_blink`

These fixtures are intentionally minimal and validate pathing/diagnostics only.
