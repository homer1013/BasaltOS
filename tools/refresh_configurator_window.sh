#!/usr/bin/env bash
set -euo pipefail

# Hard-refresh the visible BasaltOS configurator browser tab via X11.
# Usage:
#   tools/refresh_configurator_window.sh
#   tools/refresh_configurator_window.sh "BasaltOS Configurator"

pattern="${1:-BasaltOS Configurator}"

if ! command -v xdotool >/dev/null 2>&1; then
  echo "FAIL: xdotool is required"
  exit 1
fi

target="$(xdotool search --onlyvisible --name "$pattern" | head -n 1 || true)"
if [[ -z "$target" ]]; then
  echo "FAIL: no visible window matching pattern: $pattern"
  exit 1
fi

original="$(xdotool getactivewindow || true)"

xdotool windowactivate --sync "$target"
xdotool key --window "$target" ctrl+shift+r
sleep 0.2

if [[ -n "${original:-}" ]]; then
  xdotool windowactivate --sync "$original" || true
fi

title="$(xdotool getwindowname "$target" || true)"
echo "OK: refreshed $title"
