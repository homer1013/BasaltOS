#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

html = Path("tools/basaltos_config_gui.html").read_text(encoding="utf-8")

required_snippets = [
    'id="board-list" role="listbox" tabindex="0"',
    'id="board-list-status" class="sr-only" role="status" aria-live="polite"',
    'id="board-search" aria-label="Search boards" aria-controls="board-list"',
    "function handleBoardListKeydown(event)",
    "function focusBoardKeyboardIndex(nextIndex, { select = false } = {})",
    "li.setAttribute('role', 'option');",
    "li.setAttribute('aria-selected', li.classList.contains('selected') ? 'true' : 'false');",
    "announceBoardListStatus(boardKeyboardFlatItems.length, groupEntries.length);",
]

missing = [s for s in required_snippets if s not in html]
if missing:
    raise SystemExit("missing board picker a11y contract snippet(s):\n- " + "\n- ".join(missing))

print("PASS: board picker accessibility smoke checks")
PY
