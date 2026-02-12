#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
import json,glob,sys
missing=[]
for bf in sorted(glob.glob('boards/*/*/board.json')):
    with open(bf,'r',encoding='utf-8') as f:
        d=json.load(f)
    for k in ('manufacturer','architecture','family'):
        v=d.get(k)
        if not isinstance(v,str) or not v.strip():
            missing.append((bf,k))
if missing:
    print('FAIL: taxonomy fields missing:')
    for bf,k in missing:
        print(f'- {bf}: {k}')
    sys.exit(1)
print('PASS: board taxonomy fields smoke checks passed')
PY
