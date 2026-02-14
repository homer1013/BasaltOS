# Multi-Runtime Architecture Contract (Foundation)

This document defines the foundation contract for supporting multiple app runtimes in BasaltOS_Main, starting with:

- `python` (MicroPython, existing)
- `lua` (planned alternative runtime)

## Goals (Foundation Slice)

1. Keep current Python runtime behavior stable.
2. Add a runtime dispatch boundary so app launch paths are not hardwired to one runtime.
3. Standardize runtime lifecycle semantics across runtimes:
   - `start`
   - `stop`
   - `is_running`
   - `is_ready`
   - `last_result`
   - `last_error`
4. Drive runtime selection from app metadata (`app.toml`) with sane defaults.

## Non-Goals (This Slice)

1. Full Lua VM integration and production-grade bindings.
2. Full API parity between Python `basalt.*` and Lua modules.
3. Cross-platform Lua enablement beyond initial ESP32-focused bring-up path.
4. Performance tuning and hardening beyond initial guardrails.

## Runtime Selection Contract

- `app.toml` may specify `runtime = "python"` or `runtime = "lua"`.
- Missing/invalid runtime defaults to `python` for backward compatibility.
- Direct script execution without `app.toml` uses extension inference:
  - `.py` -> `python`
  - `.lua` -> `lua`
  - otherwise default `python`

## Dispatch Boundary

Shell and autorun flows must call a runtime-dispatch layer, not runtime-specific code directly.

Required dispatch API:

- `runtime_dispatch_start_file(kind, path, err, err_len)`
- `runtime_dispatch_stop(force, err, err_len)`
- `runtime_dispatch_is_running()`
- `runtime_dispatch_is_ready()`
- `runtime_dispatch_current_app()`
- `runtime_dispatch_last_result()`
- `runtime_dispatch_last_error()`
- `runtime_dispatch_last_runtime()`

## Behavior Requirements

1. A single app task remains the baseline execution model.
2. Unsupported runtime selection must fail with a clear, actionable error.
3. Shell `logs` output must report runtime identity and latest result/error in a stable format.
4. Runtime integration must not break existing Python app workflows.

## Planned Next Slices

1. Add Lua component skeleton and build gating.
2. Implement `lua_runtime` API parity with Python lifecycle semantics.
3. Add minimal Lua bindings (`system`, `gpio`, `timer`) and sample apps.
4. Add CI + bench validation lanes for runtime selection and execution.

## Build Graph Gate (Current)

- CMake option: `BASALT_ENABLE_LUA_RUNTIME` (default `OFF`)
- When enabled, build includes component:
  - `runtime/lua/lua_embed`
- This first slice only adds component topology and stub lifecycle API (`lua_embed_*`).

## Lua Runtime API Parity (Current Status)

- `lua_runtime` lifecycle interface now exists in `main/` and mirrors Python runtime surface:
  - `init`
  - `is_ready`
  - `run_file`
  - `start_file`
  - `stop`
  - `is_running`
  - `current_app`
  - `last_result`
  - `last_error`
- In this phase, Lua runtime execution intentionally returns a clear \"not integrated\" failure.
- Runtime dispatch is wired to call `lua_runtime` APIs for `runtime=lua` paths.

## Lua Binding Contract (Current Status)

- The Lua embed component now exposes a minimal binding contract in:
  - `runtime/lua/lua_embed/include/basalt_lua_bindings.h`
- Current binding surface:
  - `system.log(msg)`
  - `gpio.mode(pin, mode)` where mode is `0=input`, `1=output`, `2=open_drain`
  - `gpio.write(pin, value)`
  - `gpio.read(pin)`
  - `timer.sleep_ms(ms)`
- Contract implementation currently routes through HAL-oriented wrappers and is intended as
  a stable foundation for upcoming VM integration slices.

## Lua Guardrails (Current Status)

- Initial pre-execution guardrails are now enforced in `main/lua_runtime.c`:
  - script must exist and be non-empty
  - script size must be <= `64 KiB` (foundation default)
  - available 8-bit heap must be >= `32 KiB` before run/start
- Guardrail failures use explicit diagnostics and `last_result = "guardrail-blocked"` so shell
  status remains actionable while VM integration is still in progress.

## Shell Runtime Status (Current Status)

- `logs` output now reports per-runtime readiness and detail lines:
  - `runtime.ready.python`, `runtime.ready.lua`
  - `runtime.ready_detail.python`, `runtime.ready_detail.lua`
- Guardrail-triggered Lua failures provide an extra shell hint line:
  - `runtime.guardrail_hint: check Lua script size and free heap thresholds`
