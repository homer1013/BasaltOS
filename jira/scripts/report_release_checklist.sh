#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATE="$(date +%F)"
OUT_DIR="$ROOT/reports"
mkdir -p "$OUT_DIR"

EXPORT_DIR="${1:-}"
if [[ -z "$EXPORT_DIR" ]]; then
  # Use the latest dated export directory (YYYY-MM-DD)
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

OUT_FILE="$OUT_DIR/release_checklist_${DATE}.md"

python3 - "$ISSUES_JSON" "$OUT_FILE" <<'PY'
import json
import sys
import datetime

issues_path, out_path = sys.argv[1], sys.argv[2]
with open(issues_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

all_issues = data.get('issues', [])
release_issues = [
    i for i in all_issues
    if 'release:v0.1.0' in (i.get('fields', {}).get('labels') or [])
]

open_release = [i for i in release_issues if (i.get('fields', {}).get('status', {}).get('name') or '') != 'Done']
done_release = [i for i in release_issues if (i.get('fields', {}).get('status', {}).get('name') or '') == 'Done']

areas = ["area:cli", "area:configurator", "area:metadata", "area:docs"]
area_counts = {a: 0 for a in areas}
area_open = {a: 0 for a in areas}
for i in release_issues:
    labels = set(i.get('fields', {}).get('labels') or [])
    for area in areas:
        if area in labels:
            area_counts[area] += 1
            if (i.get('fields', {}).get('status', {}).get('name') or '') != 'Done':
                area_open[area] += 1

pts = lambda i: i.get('fields', {}).get('customfield_10016') or 0
open_points = sum(float(pts(i)) for i in open_release if isinstance(pts(i), (int, float)))
done_points = sum(float(pts(i)) for i in done_release if isinstance(pts(i), (int, float)))

checklist = [
    ("CLI reliability", area_open["area:cli"] == 0, area_open["area:cli"]),
    ("Configurator local-mode scope", area_open["area:configurator"] == 0, area_open["area:configurator"]),
    ("Metadata quality", area_open["area:metadata"] == 0, area_open["area:metadata"]),
    ("Release docs/process", area_open["area:docs"] == 0, area_open["area:docs"]),
]

ready = len(open_release) == 0

def f(i):
    return i.get('fields', {})

prio_order = {"Highest": 5, "High": 4, "Medium": 3, "Low": 2, "Lowest": 1}
open_sorted = sorted(
    open_release,
    key=lambda i: (
        -prio_order.get((f(i).get('priority') or {}).get('name', 'Medium'), 3),
        i.get('key', '')
    )
)

lines = []
lines.append(f"# v0.1.0 Release Checklist Snapshot ({datetime.date.today().isoformat()})")
lines.append("")
lines.append(f"Source export: `{issues_path}`")
lines.append("")
lines.append("## Overall Status")
lines.append(f"- Release readiness: {'READY' if ready else 'NOT READY'}")
lines.append(f"- Release-tagged issues: {len(release_issues)}")
lines.append(f"- Open release issues: {len(open_release)}")
lines.append(f"- Done release issues: {len(done_release)}")
lines.append(f"- Open story points: {open_points:.1f}")
lines.append(f"- Done story points: {done_points:.1f}")
lines.append("")
lines.append("## Gate Checklist")
for name, passed, open_count in checklist:
    lines.append(f"- [{'x' if passed else ' '}] {name} (open: {open_count})")
lines.append("")
lines.append("## Area Breakdown")
for area in areas:
    lines.append(f"- {area}: total={area_counts[area]}, open={area_open[area]}")

lines.append("")
lines.append("## Open Release Issues")
if not open_sorted:
    lines.append("- None")
else:
    for i in open_sorted:
        fields = f(i)
        key = i.get('key', '')
        status = (fields.get('status') or {}).get('name', '-')
        priority = (fields.get('priority') or {}).get('name', '-')
        assignee = (fields.get('assignee') or {}).get('displayName', 'Unassigned')
        due = fields.get('duedate') or '-'
        summary = fields.get('summary', '')
        lines.append(f"- {key} [{status}] (priority={priority}, assignee={assignee}, due={due}) {summary}")

lines.append("")
lines.append("## Notes")
lines.append("- This report is derived from local export JSON for deterministic daily snapshots.")
lines.append("- Re-run `bash jira/scripts/export_daily.sh` before regenerating this checklist.")

with open(out_path, 'w', encoding='utf-8') as f:
    f.write("\n".join(lines) + "\n")

print(out_path)
PY

echo "Wrote: $OUT_FILE"
