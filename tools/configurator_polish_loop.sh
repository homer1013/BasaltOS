#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ITERATIONS="${1:-3}"
CAM_DEV="${2:-/dev/video2}"
OUT_DIR="${3:-/tmp/basalt_cam}"

mkdir -p "$OUT_DIR"
cd "$ROOT"

for ((i=1; i<=ITERATIONS; i++)); do
  echo "[loop] iteration $i/$ITERATIONS"
  node tools/e2e/smoke_configurator.js
  node tools/e2e/deep_configurator.js
  bash tools/tests/module_picker_accessibility_smoke.sh
  bash tools/tests/board_picker_accessibility_smoke.sh

  if [[ -x tools/refresh_configurator_window.sh ]]; then
    tools/refresh_configurator_window.sh || true
  fi

  TS="$(date +%Y%m%d_%H%M%S)"
  OUT="$OUT_DIR/polish_${TS}_iter${i}.jpg"
  python3 - "$CAM_DEV" "$OUT" <<'PY'
import sys
import gi
from pathlib import Path
gi.require_version('Gst','1.0')
from gi.repository import Gst
Gst.init(None)
dev = sys.argv[1]
out = Path(sys.argv[2])
p = Gst.parse_launch(f'v4l2src device={dev} num-buffers=1 ! videoconvert ! jpegenc ! filesink location={out}')
b = p.get_bus()
p.set_state(Gst.State.PLAYING)
while True:
    m = b.timed_pop_filtered(5 * Gst.SECOND, Gst.MessageType.ERROR | Gst.MessageType.EOS)
    if m is None:
        break
    if m.type == Gst.MessageType.ERROR:
        e, _ = m.parse_error()
        print(f"capture_error:{e}")
        break
    if m.type == Gst.MessageType.EOS:
        print(f"capture_ok:{out}")
        break
p.set_state(Gst.State.NULL)
PY
done

echo "[loop] done"
