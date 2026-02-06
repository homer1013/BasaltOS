# MicroPython runtime plan (v0.1)

Basalt OS will embed MicroPython as the first app runtime.

## Approach
- Build MicroPython as a component under `runtime/python/`.
- Expose Basalt APIs as MicroPython modules (C bindings).
- App loader maps `app.toml` + `main.py` into runtime execution.

## v0.1 scope
- REPL on UART
- `basalt` module with `gpio` + `timer` first
- Run app scripts from SPIFFS-backed `/data` storage

## Current status
- MicroPython embed runtime integrated (minimal config)
- `bsh run <path>` reads file and executes it via `mp_embed_exec_str`
- `basalt` module available (`gpio`, `timer`, `led`, and `ui`)
- `basalt.ui` now exposes primitives used by first-party apps:
  - `ready`, `clear`, `text`, `text_at`, `color`
  - `pixel`, `line`, `rect`, `circle`, `ellipse`
  - `touch`

## Running scripts
`spiffs/` is baked into the `storage` partition at build time. Example:
```
run /data/hello.py
```

## Runtime execution contract (current)
- `run`/`run_dev` starts a single app task; concurrent app tasks are rejected.
- `runtime.last_result` tracks coarse lifecycle state:
  - `running` on successful task start
  - `completed` when script execution returns without runtime-level error
  - `failed` when load/execute/start/stop paths fail
  - `stopped-by-user` / `killed-by-user` for shell stop actions
- `runtime.last_error` stores the latest runtime error string for diagnostics.
- Task-time script startup failures are printed to shell output as:
  - `run: <error>`
- Runtime diagnostics are available via shell `logs`.
- Runtime history (`last_result`, `last_error`) is RAM-only and resets on reboot.

## Regenerating the embed package
If you update the MicroPython repo, regenerate the embed package:
```
./tools/gen_mpy_embed.sh
```
