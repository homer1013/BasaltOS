#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

echo "[hardening] 1/6 configure invalid-board regression"
bash tools/tests/configure_invalid_board_regression.sh

echo "[hardening] 2/6 shell UX contract"
bash tools/tests/shell_ux_polish_smoke.sh

echo "[hardening] 3/6 quickstart contract"
bash tools/tests/esp32_first_success_quickstart_smoke.sh

echo "[hardening] 4/6 examples pack contract"
bash tools/tests/examples_pack_recipes_smoke.sh

echo "[hardening] 5/6 staged artifact guard"
bash tools/tests/staged_artifact_guard.sh

echo "[hardening] 6/6 configurator API smoke"
bash tools/tests/configurator_api_smoke.sh

echo "PASS: main hardening smoke bundle"
