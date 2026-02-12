#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

F="main/app_main.c"

search_q() {
  local pattern="$1"
  local file="$2"
  if command -v rg >/dev/null 2>&1; then
    rg -q -- "$pattern" "$file"
  else
    grep -Eq -- "$pattern" "$file"
  fi
}

search_n() {
  local pattern="$1"
  local file="$2"
  if command -v rg >/dev/null 2>&1; then
    rg -n -- "$pattern" "$file"
  else
    grep -En -- "$pattern" "$file"
  fi
}

require() {
  local pattern="$1"
  local msg="$2"
  if ! search_q "$pattern" "$F"; then
    echo "FAIL: $msg"
    exit 1
  fi
}

require "static void bsh_print_usage\\(" "missing centralized usage helper"
require "static void bsh_print_unknown_subcommand\\(" "missing unknown subcommand helper"
require "static void bsh_print_unknown_command\\(" "missing unknown command helper"
require "hint: use 'help -commands'" "missing unknown-command hint"
require "run: missing required argument <app\\|script>" "run missing-arg message not normalized"
require "run_dev: missing required argument <app\\|script>" "run_dev missing-arg message not normalized"

sub_calls=$(search_n "bsh_print_unknown_subcommand\\(" "$F" | wc -l | tr -d ' ')
if [[ "$sub_calls" -lt 8 ]]; then
  echo "FAIL: expected at least 8 bsh_print_unknown_subcommand references (helper + callsites), got $sub_calls"
  exit 1
fi

echo "PASS: shell UX normalization smoke checks passed"
