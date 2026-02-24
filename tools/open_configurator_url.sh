#!/usr/bin/env bash
set -euo pipefail

# Open/navigate the visible configurator Firefox window to a specific URL.
# Example:
#   tools/open_configurator_url.sh "http://localhost:5000/?open=config&step=2"

url="${1:-http://localhost:5000/}"
pattern="${2:-BasaltOS Configurator}"

if ! command -v xdotool >/dev/null 2>&1; then
  echo "FAIL: xdotool is required"
  exit 1
fi

win="$(xdotool search --onlyvisible --name "$pattern" | head -n 1 || true)"
if [[ -z "$win" ]]; then
  echo "FAIL: no visible window matching pattern: $pattern"
  exit 1
fi

original="$(xdotool getactivewindow || true)"
xdotool windowactivate --sync "$win"
xdotool key --window "$win" ctrl+l
sleep 0.08
xdotool type --window "$win" --delay 1 "$url"
xdotool key --window "$win" Return
sleep 0.2

if [[ -n "${original:-}" ]]; then
  xdotool windowactivate --sync "$original" || true
fi

title="$(xdotool getwindowname "$win" || true)"
echo "OK: navigated $title -> $url"
