#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

html = Path("tools/basaltos_config_gui.html").read_text(encoding="utf-8")

required = [
    'id="module-grid-status" class="sr-only" role="status" aria-live="polite"',
    "function syncModuleCardAccessibility(module, card, toggle, enabled, allowed)",
    "card.setAttribute('role', 'checkbox');",
    "card.setAttribute('aria-checked', enabled ? 'true' : 'false');",
    "card.onclick = (event) => {",
    "card.onkeydown = (event) => {",
    "if (event.key === 'Enter' || event.key === ' ')",
    "announceModuleGridStatus(",
]

missing = [s for s in required if s not in html]
if missing:
    raise SystemExit("missing module picker a11y contract snippet(s):\n- " + "\n- ".join(missing))

print("PASS: module picker accessibility smoke checks")
PY
