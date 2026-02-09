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

OUT_FILE="$OUT_DIR/daily_status_${DATE}.md"
SOCIAL_FILE="$OUT_DIR/daily_social_${DATE}.md"

python3 - "$ISSUES_JSON" "$OUT_FILE" "$SOCIAL_FILE" "$DATE" <<'PY'
import json
import sys

issues_path, out_path, social_path, date = sys.argv[1:]
with open(issues_path, "r", encoding="utf-8") as f:
    data = json.load(f)

issues = data.get("issues", [])

def fields(issue):
    return issue.get("fields", {})

def status_name(issue):
    return (fields(issue).get("status") or {}).get("name", "")

def labels(issue):
    return set(fields(issue).get("labels") or [])

def points(issue):
    val = fields(issue).get("customfield_10016")
    return float(val) if isinstance(val, (int, float)) else 0.0

release = [i for i in issues if "release:v0.1.0" in labels(i)]
open_release = [i for i in release if status_name(i) != "Done"]
done_release = [i for i in release if status_name(i) == "Done"]

in_progress = [i for i in open_release if status_name(i) in {"In Progress", "In Review"}]
todo = [i for i in open_release if status_name(i) not in {"In Progress", "In Review"}]

prio_order = {"Highest": 5, "High": 4, "Medium": 3, "Low": 2, "Lowest": 1}
def sort_key(issue):
    f = fields(issue)
    prio = (f.get("priority") or {}).get("name", "Medium")
    return (-prio_order.get(prio, 3), issue.get("key", ""))

top_next = sorted(todo, key=sort_key)[:5]

open_points = sum(points(i) for i in open_release)
done_points = sum(points(i) for i in done_release)

lines = []
lines.append(f"# Daily Status ({date})")
lines.append("")
lines.append(f"Source export: `{issues_path}`")
lines.append("")
lines.append("## Snapshot")
lines.append(f"- Release-tagged issues: {len(release)}")
lines.append(f"- Done: {len(done_release)} ({done_points:.1f} pts)")
lines.append(f"- Open: {len(open_release)} ({open_points:.1f} pts)")
lines.append(f"- In progress/review: {len(in_progress)}")
lines.append("")
lines.append("## In Progress")
if not in_progress:
    lines.append("- None")
else:
    for issue in sorted(in_progress, key=sort_key):
        f = fields(issue)
        assignee = (f.get("assignee") or {}).get("displayName", "Unassigned")
        lines.append(f"- {issue.get('key')} [{status_name(issue)}] ({assignee}) {f.get('summary', '')}")
lines.append("")
lines.append("## Next Up (Top 5)")
if not top_next:
    lines.append("- None")
else:
    for issue in top_next:
        f = fields(issue)
        due = f.get("duedate") or "-"
        lines.append(f"- {issue.get('key')} (due={due}) {f.get('summary', '')}")
lines.append("")
lines.append("## Suggested EOD Comment Template")
lines.append("- Completed: <keys + one-line outcome>")
lines.append("- In progress: <keys + one-line status>")
lines.append("- Next: <keys>")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

social = []
social.append(f"# BasaltOS Daily Update ({date})")
social.append("")
social.append("Build update:")
social.append(f"- v0.1.0 release scope: {len(done_release)}/{len(release)} tasks done")
social.append(f"- Active right now: {len(in_progress)} task(s) in progress/review")
if top_next:
    social.append(f"- Next focus: {top_next[0].get('key')} {fields(top_next[0]).get('summary', '')}")
social.append("")
social.append("Following along:")
social.append("- Star/watch on GitHub")
social.append("- Sponsor: https://github.com/sponsors/homer1013")
social.append("- Buy me a coffee: https://buymeacoffee.com/homer.morrill")

with open(social_path, "w", encoding="utf-8") as f:
    f.write("\n".join(social) + "\n")

print(out_path)
print(social_path)
PY

echo "Wrote: $OUT_FILE"
echo "Wrote: $SOCIAL_FILE"
