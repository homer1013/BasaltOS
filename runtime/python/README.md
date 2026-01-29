# MicroPython runtime (embedded)

MicroPython is embedded using the official `ports/embed` output.

## Current integration
- Embedded VM with minimal config
- `bsh run /apps/<app>/main.py` executes via `mp_embed_exec_str`

## Basalt module
`import basalt` provides:
- `basalt.gpio.mode(pin, mode)` (mode: 0=input, 1=output)
- `basalt.gpio.set(pin, value)`
- `basalt.gpio.get(pin)`
- `basalt.timer.sleep_ms(ms)`
## Next steps
- Expose `basalt` module (gpio, timer, fs, events)
- Add REPL or persistent runtime task

## Regeneration
Run `./tools/gen_mpy_embed.sh` after updating the MicroPython repo.
