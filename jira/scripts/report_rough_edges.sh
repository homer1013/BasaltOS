#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATE="$(date +%F)"
OUT_DIR="$ROOT/reports"
mkdir -p "$OUT_DIR"

EXPORT_DIR="${1:-}"
if [[ -z "$EXPORT_DIR" ]]; then
  EXPORT_DIR="$(find "$ROOT/exports" -mindepth 1 -maxdepth 1 -type d -name '20*' | sort | tail -n 1)"
fi

if [[ -z "$EXPORT_DIR" || ! -d "$EXPORT_DIR" ]]; then
  echo "No export directory found. Run: bash jira/scripts/export_daily.sh" >&2
  exit 1
fi

ISSUES_JSON="$EXPORT_DIR/issues.json"
if [[ ! -f "$ISSUES_JSON" ]]; then
  echo "Missing issues export: $ISSUES_JSON" >&2
  exit 1
fi

OUT_FILE="$OUT_DIR/rough_edges_${DATE}.md"

python3 - "$ISSUES_JSON" "$OUT_FILE" <<'PY'
import json
import sys
import datetime

issues_path, out_path = sys.argv[1], sys.argv[2]
with open(issues_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

issues = data.get('issues', [])

def fields(i):
    return i.get('fields', {})

def status(i):
    return (fields(i).get('status') or {}).get('name', '')

def labels(i):
    return set(fields(i).get('labels') or [])

def points(i):
    v = fields(i).get('customfield_10016')
    return float(v) if isinstance(v, (int, float)) else 0.0

cand = []
for i in issues:
    if status(i) == 'Done':
        continue
    ls = labels(i)
    if 'area:cli' in ls or 'area:configurator' in ls:
        cand.append(i)

prio_order = {'Highest': 5, 'High': 4, 'Medium': 3, 'Low': 2, 'Lowest': 1}

def sk(i):
    f = fields(i)
    pr = (f.get('priority') or {}).get('name', 'Medium')
    return (-prio_order.get(pr, 3), i.get('key', ''))

cand_sorted = sorted(cand, key=sk)
top5 = cand_sorted[:5]

open_pts = sum(points(i) for i in cand)
top_pts = sum(points(i) for i in top5)

lines = []
lines.append(f"# CLI/Configurator Rough-Edge Burn-Down ({datetime.date.today().isoformat()})")
lines.append('')
lines.append(f"Source export: `{issues_path}`")
lines.append('')
lines.append('## Summary')
lines.append(f"- Open CLI/configurator items: {len(cand)}")
lines.append(f"- Open points (CLI/configurator): {open_pts:.1f}")
lines.append(f"- Top-5 focus points: {top_pts:.1f}")
lines.append('')
lines.append('## Top 5 Focus')
if not top5:
    lines.append('- None')
else:
    for i in top5:
        f = fields(i)
        key = i.get('key', '')
        st = status(i)
        pr = (f.get('priority') or {}).get('name', '-')
        ass = (f.get('assignee') or {}).get('displayName', 'Unassigned')
        due = f.get('duedate') or '-'
        lines.append(f"- {key} [{st}] (priority={pr}, assignee={ass}, due={due}) {f.get('summary','')}")

lines.append('')
lines.append('## Daily Burn-Down Actions')
lines.append('- Keep only one rough-edge issue in `In Progress` at a time.')
lines.append('- When fixed: add evidence (command output/test), transition to `Done`, regenerate this report.')
lines.append('- If blocked: add blocker comment with owner/date and move to next top item.')

with open(out_path, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')

print(out_path)
PY

echo "Wrote: $OUT_FILE"
