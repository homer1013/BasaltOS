#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PORT="${BASALT_PORT:-${1:-/dev/ttyACM0}}"
BUILD_DIR="${BASALT_BUILD_DIR:-build_m5stickc_plus2}"
SKIP_FLASH="${BASALT_SKIP_FLASH:-0}"
SKIP_CONFIGURE="${BASALT_SKIP_CONFIGURE:-0}"

if [[ ! -e "$PORT" ]]; then
  echo "SKIP: serial port not found: $PORT"
  exit 0
fi

# shellcheck source=/dev/null
source tools/env.sh >/dev/null

if [[ "$SKIP_CONFIGURE" != "1" ]]; then
  python3 tools/configure.py \
    --platform esp32 \
    --board m5stickc_plus2 \
    --quickstart m5stickc_plus2
fi

if [[ "$SKIP_FLASH" != "1" ]]; then
  idf.py -B "$BUILD_DIR" set-target esp32
  idf.py -B "$BUILD_DIR" build
  idf.py -B "$BUILD_DIR" -p "$PORT" flash
fi

python3 - "$PORT" <<'PY'
import time
import sys

import serial

port = sys.argv[1]

commands = [
    "help tft",
    "tft status",
    "tft fill red",
    "tft fill blue",
    "tft clear",
    "tft text 0 0 BASALT TFT OK",
    "devcheck",
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
    ser.write(b"\n")
    time.sleep(0.25)
    _ = read_for(ser, 1.2)

    collected = []
    for cmd in commands:
        ser.write((cmd + "\n").encode("utf-8"))
        time.sleep(0.25)
        out = read_for(ser, 1.8)
        print(f"\n>>> {cmd}\n{out}")
        collected.append((cmd, out))

joined = "\n".join(out for _, out in collected)

needles = [
    ("tft [status|clear|fill", "missing tft help surface"),
    ("tft: ready", "tft status did not report ready"),
    ("ok: tft fill red", "missing red fill ack"),
    ("ok: tft fill blue", "missing blue fill ack"),
    ("ok: tft clear", "missing clear ack"),
    ("ok: tft text", "missing text ack"),
    ("devcheck: tft_console=ready", "devcheck did not report tft_console ready"),
]

for needle, msg in needles:
    if needle not in joined:
        raise SystemExit(f"FAIL: {msg}")

print("PASS: M5StickC Plus2 TFT bench smoke checks")
PY
