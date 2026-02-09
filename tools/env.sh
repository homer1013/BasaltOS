#!/usr/bin/env bash
# Source this to set up ESP-IDF environment for the current shell.

candidates=(
  "${IDF_PATH:-}"
  "$HOME/esp-idf"
  "$HOME/esp/esp-idf"
  "$HOME/Projects/esp/esp-idf"
  "$HOME/Projects/esp-idf"
)

for base in "${candidates[@]}"; do
  [[ -z "$base" ]] && continue
  [[ -f "$base/export.sh" ]] || continue

  # Probe in a subshell first so failed exports don't pollute current shell.
  if bash -lc "source '$base/export.sh' >/dev/null 2>&1 && idf.py --version >/dev/null 2>&1"; then
    # shellcheck disable=SC1090
    source "$base/export.sh"
    echo "ESP-IDF environment loaded from: $base"
    return 0
  fi

done

echo "ESP-IDF export.sh found, but no usable installation passed validation." >&2
echo "Checked:" >&2
for base in "${candidates[@]}"; do
  [[ -z "$base" ]] && continue
  echo "  - $base/export.sh" >&2
done
return 1
