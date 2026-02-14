#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

PORT="${BASALT_PORT:-${1:-/dev/ttyACM0}}"
BUILD_DIR="${BASALT_BUILD_DIR:-build_c6_lua_bench}"

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

SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B "$BUILD_DIR" -D BASALT_ENABLE_LUA_RUNTIME=ON set-target esp32c6
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B "$BUILD_DIR" -D BASALT_ENABLE_LUA_RUNTIME=ON build
idf.py -B "$BUILD_DIR" -p "$PORT" flash

python3 - "$PORT" <<'PY'
import re
import sys
import time

import serial

port = sys.argv[1]

commands = [
    "logs",
    "run /apps/lua_hello",
    "run /apps/lua_blink",
    "logs",
    "stop",
    "logs",
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
    _ = read_for(ser, 1.2)

    full = []
    for cmd in commands:
        ser.write((cmd + "\n").encode())
        time.sleep(0.2)
        out = read_for(ser, 2.0)
        print(f"\n>>> {cmd}\n{out}")
        full.append((cmd, out))

joined = "\n".join(out for _, out in full)

if "runtime.ready.lua: yes" not in joined:
    raise SystemExit("FAIL: logs missing runtime.ready.lua: yes")
if "run: launching /data/apps/lua_hello/main.lua (lua)" not in joined:
    raise SystemExit("FAIL: lua_hello launch line missing")
if "run: launching /data/apps/lua_blink/main.lua (lua)" not in joined:
    raise SystemExit("FAIL: lua_blink launch line missing")
if "lua vm execution is not integrated in this build yet" not in joined:
    raise SystemExit("FAIL: expected Lua VM-not-integrated diagnostic missing")
if "runtime.last_runtime: lua" not in joined:
    raise SystemExit("FAIL: logs missing runtime.last_runtime: lua")
if "runtime.last_result: failed" not in joined:
    raise SystemExit("FAIL: logs missing runtime.last_result: failed")
if "stop: no app running" not in joined:
    raise SystemExit("FAIL: expected stop behavior line missing")

m = re.findall(r"runtime\.ready_detail\.lua:\s*(.+)", joined)
if not m:
    raise SystemExit("FAIL: logs missing runtime.ready_detail.lua line")

print("PASS: ESP32-C6 Lua runtime bench smoke checks")
PY
