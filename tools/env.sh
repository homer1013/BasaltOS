#!/usr/bin/env bash
# Source this to set up ESP-IDF environment for the current shell.

if [[ -d "$HOME/esp-idf" ]]; then
  # shellcheck disable=SC1091
  source "$HOME/esp-idf/export.sh"
else
  echo "ESP-IDF not found at $HOME/esp-idf"
  return 1
fi
