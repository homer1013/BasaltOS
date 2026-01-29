# MicroPython runtime plan (v0.1)

Basalt OS will embed MicroPython as the first app runtime.

## Approach
- Build MicroPython as a component under `runtime/python/`.
- Expose Basalt APIs as MicroPython modules (C bindings).
- App loader maps `app.toml` + `main.py` into runtime execution.

## v0.1 scope
- REPL on UART
- `basalt` module with `gpio` + `timer` first
- Run `apps/*.app/main.py` from LittleFS

## Current status
- MicroPython embed runtime integrated (minimal config)
- `bsh run <path>` reads file and executes it via `mp_embed_exec_str`
- `basalt` module available (gpio + timer)

## Running scripts
`spiffs/` is baked into the `storage` partition at build time. Example:
```
run /data/hello.py
```

## Regenerating the embed package
If you update the MicroPython repo, regenerate the embed package:
```
./tools/gen_mpy_embed.sh
```
