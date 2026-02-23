#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PORT="${BASALT_UNO_PORT:-${BASALT_PORT:-${1:-/dev/ttyACM0}}}"
FQBN="${BASALT_UNO_FQBN:-arduino:renesas_uno:unor4wifi}"
SKETCH_DIR="modules/tft_parallel_uno/runtime/uno_r4_tft_parallel_runtime"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "SKIP: arduino-cli not installed"
  exit 0
fi

if [[ ! -e "$PORT" ]]; then
  echo "SKIP: serial port not found: $PORT"
  exit 0
fi

arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR" >/tmp/uno_r4_tft_parallel_compile.log
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH_DIR" >/tmp/uno_r4_tft_parallel_upload.log

python3 - "$PORT" <<'PY'
import sys
import time

import serial

port = sys.argv[1]

commands = [
    "help",
    "lcd id",
    "fill red",
    "fill blue",
]

def read_for(ser, sec):
    end = time.time() + sec
    out = []
    while time.time() < end:
        d = ser.read(ser.in_waiting or 1)
        if d:
            out.append(d.decode("utf-8", "replace"))
        else:
            time.sleep(0.02)
    return "".join(out)

with serial.Serial(port, baudrate=115200, timeout=0.1) as ser:
    time.sleep(2.0)
    ser.write(b"\n")
    boot = read_for(ser, 2.0)
    print("\n>>> boot\n" + boot)

    full = [boot]
    for cmd in commands:
        ser.write((cmd + "\n").encode())
        time.sleep(0.25)
        out = read_for(ser, 1.8)
        print(f"\n>>> {cmd}\n{out}")
        full.append(out)

    tail = read_for(ser, 1.0)
    if tail:
        print(f"\n>>> tail\n{tail}")
        full.append(tail)

joined = "\n".join(full).lower()

if "lcd id: 0x" not in joined:
    raise SystemExit("FAIL: lcd id line missing")
if "commands: help, lcd id, fill red|green|blue|black|white" not in joined:
    raise SystemExit("FAIL: help output missing")
if "ok: fill red" not in joined:
    raise SystemExit("FAIL: fill red acknowledgement missing")
if "ok: fill blue" not in joined:
    raise SystemExit("FAIL: fill blue acknowledgement missing")

print("PASS: UNO R4 tft_parallel_uno bench smoke checks")
PY
