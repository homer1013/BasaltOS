#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
from pathlib import Path

ports = [
    Path('basalt_hal/ports/esp32'),
    Path('basalt_hal/ports/esp32c3'),
    Path('basalt_hal/ports/esp32c6'),
    Path('basalt_hal/ports/esp32s3'),
]
issues = []

for pdir in ports:
    for src in sorted(pdir.glob('hal_*.c')):
        text = src.read_text(encoding='utf-8')
        if text.lstrip().startswith('#pragma once'):
            continue
        if '_impl_t' not in text:
            continue
        if '_Static_assert(sizeof(' not in text or '->_opaque' not in text:
            issues.append(f"{src}: missing opaque storage _Static_assert")

if issues:
    raise SystemExit('FAIL: opaque handle storage assertion gaps:\n' + '\n'.join(issues))

print('PASS: HAL opaque handle assertion smoke checks')
PY
