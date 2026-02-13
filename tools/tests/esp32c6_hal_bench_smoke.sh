#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PORT="${BASALT_PORT:-${1:-/dev/ttyACM0}}"
BUILD_DIR="${BASALT_BUILD_DIR:-build_c6_hal_bench}"
RMT_REQUIRED="${BASALT_RMT_LOOPBACK_REQUIRED:-1}"
UART_REQUIRED="${BASALT_UART_LOOPBACK_REQUIRED:-0}"

if [[ ! -e "$PORT" ]]; then
  echo "SKIP: serial port not found: $PORT"
  exit 0
fi

# shellcheck source=/dev/null
source tools/env.sh

python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c6 \
  --template esp32_c6_rmt_loopback \
  --enable-drivers gpio,uart,i2c,spi,fs_spiffs,shell_full,timer,rmt,i2s,mic,adc,pwm \
  --applets system_info

SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B "$BUILD_DIR" set-target esp32c6
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B "$BUILD_DIR" build
idf.py -B "$BUILD_DIR" -p "$PORT" flash

python3 - "$PORT" "$RMT_REQUIRED" "$UART_REQUIRED" <<'PY'
import re
import sys
import time

import serial

port = sys.argv[1]
rmt_required = sys.argv[2] == "1"
uart_required = sys.argv[3] == "1"

commands = [
    "drivers",
    "i2c status",
    "i2c scan",
    "uart status",
    "uart loopback BASALTC6UART 500",
    "pwm status",
    "pwm start 25 1000",
    "pwm duty 60",
    "pwm stop",
    "rmt loopback 500 500 10 300",
    "mic read 300",
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
    time.sleep(0.2)
    _ = read_for(ser, 1.0)

    full = []
    for cmd in commands:
        ser.write((cmd + "\n").encode())
        time.sleep(0.2)
        out = read_for(ser, 1.8)
        print(f"\n>>> {cmd}\n{out}")
        full.append((cmd, out))

joined = "\n".join(out for _, out in full)

if "rmt: enabled" not in joined:
    raise SystemExit("FAIL: drivers output missing 'rmt: enabled'")
if "mic: enabled" not in joined:
    raise SystemExit("FAIL: drivers output missing 'mic: enabled'")
if "uart: enabled" not in joined:
    raise SystemExit("FAIL: drivers output missing 'uart: enabled'")
if "i2c: enabled" not in joined:
    raise SystemExit("FAIL: drivers output missing 'i2c: enabled'")
if "pwm: enabled" not in joined:
    raise SystemExit("FAIL: drivers output missing 'pwm: enabled'")
if "i2c.enabled: yes" not in joined:
    raise SystemExit("FAIL: i2c status missing 'i2c.enabled: yes'")
if "i2c scan:" not in joined:
    raise SystemExit("FAIL: missing i2c scan output")
if "uart.enabled: yes" not in joined:
    raise SystemExit("FAIL: uart status missing 'uart.enabled: yes'")
if "uart loopback:" not in joined:
    raise SystemExit("FAIL: missing uart loopback output")
if uart_required and " pass" not in joined:
    raise SystemExit("FAIL: uart loopback did not report pass")
if "pwm.enabled: yes" not in joined:
    raise SystemExit("FAIL: pwm status missing 'pwm.enabled: yes'")
if "pwm start:" not in joined:
    raise SystemExit("FAIL: missing pwm start output")
if "pwm duty:" not in joined:
    raise SystemExit("FAIL: missing pwm duty output")
if "pwm stop:" not in joined:
    raise SystemExit("FAIL: missing pwm stop output")
if "rmt loopback:" not in joined:
    raise SystemExit("FAIL: missing rmt loopback output")

m = re.search(r"rmt loopback: emitted=\d+ edges=(\d+) done", joined)
if not m:
    raise SystemExit("FAIL: unable to parse rmt loopback edge count")
edges = int(m.group(1))
if rmt_required and edges == 0:
    raise SystemExit("FAIL: rmt loopback edges=0 (check GPIO2->GPIO3 jumper)")

m2 = re.search(r"mic read: samples=(\d+) raw_min=([-\d]+) raw_max=([-\d]+)", joined)
if not m2:
    raise SystemExit("FAIL: unable to parse mic read sample summary")
samples = int(m2.group(1))
if samples < 20:
    raise SystemExit(f"FAIL: mic read produced too few samples ({samples})")

print("PASS: ESP32-C6 HAL bench smoke checks")
PY
