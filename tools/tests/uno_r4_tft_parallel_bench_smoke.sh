#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PORT="${BASALT_UNO_PORT:-${BASALT_PORT:-${1:-/dev/ttyACM0}}}"
FQBN="${BASALT_UNO_FQBN:-arduino:renesas_uno:unor4wifi}"
CAMERA_DEV="${BASALT_CAMERA_DEVICE:-}"
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

python3 - "$PORT" "$CAMERA_DEV" <<'PY'
import os
import subprocess
import sys
import time

import serial

port = sys.argv[1]
camera_dev = (sys.argv[2] or "").strip()

commands = [
    "help",
    "cal show",
    "cal save",
    "cal load",
    "lcd id",
    "fill red",
    "fill blue",
]

camera_ok = bool(camera_dev) and subprocess.call(
    ["bash", "-lc", "command -v ffmpeg >/dev/null 2>&1"],
    stdout=subprocess.DEVNULL,
    stderr=subprocess.DEVNULL,
) == 0

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


def capture_ppm(tag):
    if not camera_ok:
        return None
    out = f"/tmp/uno_r4_tft_parallel_{tag}.ppm"
    cmd = [
        "ffmpeg",
        "-hide_banner",
        "-loglevel",
        "error",
        "-f",
        "video4linux2",
        "-i",
        camera_dev,
        "-frames:v",
        "1",
        "-y",
        out,
    ]
    try:
        subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return out
    except Exception:
        return None


def ppm_channel_means(path):
    with open(path, "rb") as f:
        magic = f.readline().strip()
        if magic != b"P6":
            raise RuntimeError("unexpected ppm magic")
        line = f.readline().strip()
        while line.startswith(b"#"):
            line = f.readline().strip()
        w, h = [int(x) for x in line.split()]
        maxv = int(f.readline().strip())
        if maxv <= 0:
            raise RuntimeError("invalid ppm maxv")
        data = f.read(w * h * 3)
    if len(data) < w * h * 3:
        raise RuntimeError("short ppm payload")
    r = 0
    g = 0
    b = 0
    px = w * h
    for i in range(0, px * 3, 3):
        r += data[i]
        g += data[i + 1]
        b += data[i + 2]
    return (r / px, g / px, b / px)

with serial.Serial(port, baudrate=115200, timeout=0.1) as ser:
    red_ppm = None
    blue_ppm = None
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
        if cmd == "fill red":
            red_ppm = capture_ppm("red")
        if cmd == "fill blue":
            blue_ppm = capture_ppm("blue")

    tail = read_for(ser, 1.0)
    if tail:
        print(f"\n>>> tail\n{tail}")
        full.append(tail)

joined = "\n".join(full).lower()

if "lcd id: 0x" not in joined:
    raise SystemExit("FAIL: lcd id line missing")
if "commands: help, lcd id, fill red|green|blue|black|white" not in joined:
    raise SystemExit("FAIL: help output missing")
if "cal: saved" not in joined:
    raise SystemExit("FAIL: calibration save acknowledgement missing")
if "cal: loaded" not in joined:
    raise SystemExit("FAIL: calibration load acknowledgement missing")
if "ok: fill red" not in joined:
    raise SystemExit("FAIL: fill red acknowledgement missing")
if "ok: fill blue" not in joined:
    raise SystemExit("FAIL: fill blue acknowledgement missing")

if camera_ok:
    if not red_ppm or not blue_ppm:
        raise SystemExit("FAIL: camera hook enabled but frame capture failed")
    red_means = ppm_channel_means(red_ppm)
    blue_means = ppm_channel_means(blue_ppm)
    red_bias = red_means[0] - red_means[2]
    blue_bias = blue_means[2] - blue_means[0]
    if red_bias < 2.0:
        raise SystemExit(f"FAIL: red frame does not appear red-dominant (bias={red_bias:.2f})")
    if blue_bias < 2.0:
        raise SystemExit(f"FAIL: blue frame does not appear blue-dominant (bias={blue_bias:.2f})")
    print(f"camera-check: red={red_ppm} blue={blue_ppm}")

print("PASS: UNO R4 tft_parallel_uno bench smoke checks")
PY
