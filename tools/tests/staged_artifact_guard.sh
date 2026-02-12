#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPO="$ROOT"

usage() {
  cat <<'USAGE'
Usage:
  bash tools/tests/staged_artifact_guard.sh [--repo <path>]

Fails if staged files include known local/generated artifacts that should not be committed.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo)
      [[ $# -ge 2 ]] || { usage; exit 1; }
      REPO="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown arg: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if ! git -C "$REPO" rev-parse --git-dir >/dev/null 2>&1; then
  echo "FAIL: not a git repo: $REPO"
  exit 1
fi

mapfile -t staged < <(git -C "$REPO" diff --cached --name-only --diff-filter=ACMR)
if [[ "${#staged[@]}" -eq 0 ]]; then
  echo "PASS: staged artifact guard (no staged files)"
  exit 0
fi

blocked=()
for p in "${staged[@]}"; do
  case "$p" in
    config/generated|config/generated/*|build|build/*|build_*|build_*/*|.basaltos-local|.basaltos-local/*|tmp|tmp/*)
      blocked+=("$p")
      ;;
  esac
done

if [[ "${#blocked[@]}" -gt 0 ]]; then
  echo "FAIL: staged artifact guard found blocked path(s):"
  for p in "${blocked[@]}"; do
    echo "  - $p"
  done
  echo "Unstage/remove these local/generated artifacts before commit."
  exit 1
fi

echo "PASS: staged artifact guard"
