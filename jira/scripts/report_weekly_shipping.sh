#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN_ROOT="$(cd "$ROOT/.." && pwd)"
DATE="$(date +%F)"
OUT_DIR="$ROOT/reports"
mkdir -p "$OUT_DIR"

EXPORT_DIR="${1:-}"
if [[ -z "$EXPORT_DIR" ]]; then
  EXPORT_DIR="$(find "$ROOT/exports" -mindepth 1 -maxdepth 1 -type d -name '20*' | sort | tail -n 1)"
fi

DAYS="${2:-7}"
if ! [[ "$DAYS" =~ ^[0-9]+$ ]] || [[ "$DAYS" -le 0 ]]; then
  echo "Invalid days window '$DAYS' (must be positive integer)" >&2
  exit 1
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

SHIPPING_FILE="$OUT_DIR/weekly_shipping_${DATE}.md"
ALIGN_FILE="$OUT_DIR/weekly_alignment_${DATE}.md"

python3 - "$ISSUES_JSON" "$SHIPPING_FILE" "$DATE" "$DAYS" <<'PY'
import datetime as dt
import json
import sys

issues_path, out_path, date_s, days_s = sys.argv[1:]
today = dt.date.fromisoformat(date_s)
days = int(days_s)
start = today - dt.timedelta(days=days - 1)

with open(issues_path, "r", encoding="utf-8") as f:
    payload = json.load(f)
issues = payload.get("issues", [])

def fields(issue):
    return issue.get("fields", {})

def status_name(issue):
    return (fields(issue).get("status") or {}).get("name", "")

def labels(issue):
    return set(fields(issue).get("labels") or [])

def points(issue):
    val = fields(issue).get("customfield_10016")
    return float(val) if isinstance(val, (int, float)) else 0.0

def parse_jira_dt(s):
    if not s or not isinstance(s, str):
        return None
    s = s.strip()
    if not s:
        return None
    if s.endswith("Z"):
        s = s[:-1] + "+00:00"
    try:
        return dt.datetime.fromisoformat(s)
    except Exception:
        return None

def in_window(dt_val):
    if dt_val is None:
        return False
    d = dt_val.date()
    return start <= d <= today

release_tag = "release:v0.1.0"
release_issues = [i for i in issues if release_tag in labels(i)]
scope_mode = "release-tagged" if release_issues else "all-issues-fallback"
scope = release_issues if release_issues else issues

done_now = [i for i in scope if status_name(i) == "Done"]
open_now = [i for i in scope if status_name(i) != "Done"]
in_progress_now = [i for i in open_now if status_name(i) in {"In Progress", "In Review"}]

completed_this_week = []
created_this_week = []
for i in scope:
    f = fields(i)
    res = parse_jira_dt(f.get("resolutiondate"))
    cre = parse_jira_dt(f.get("created"))
    if status_name(i) == "Done" and in_window(res):
        completed_this_week.append(i)
    if in_window(cre):
        created_this_week.append(i)

prio_order = {"Highest": 5, "High": 4, "Medium": 3, "Low": 2, "Lowest": 1}
def sort_key(issue):
    f = fields(issue)
    p = (f.get("priority") or {}).get("name", "Medium")
    return (-prio_order.get(p, 3), issue.get("key", ""))

completed_this_week.sort(key=sort_key)
open_now.sort(key=sort_key)
in_progress_now.sort(key=sort_key)

done_points = sum(points(i) for i in done_now)
open_points = sum(points(i) for i in open_now)
completed_points = sum(points(i) for i in completed_this_week)

lines = []
lines.append(f"# Weekly Shipping Summary ({start.isoformat()} to {today.isoformat()})")
lines.append("")
lines.append(f"Source export: `{issues_path}`")
lines.append("")
lines.append("## Snapshot")
lines.append(f"- Scope mode: {scope_mode}")
lines.append(f"- Scope issues counted: {len(scope)}")
lines.append(f"- Done now: {len(done_now)} ({done_points:.1f} pts)")
lines.append(f"- Open now: {len(open_now)} ({open_points:.1f} pts)")
lines.append(f"- In progress/review now: {len(in_progress_now)}")
lines.append(f"- Completed this window: {len(completed_this_week)} ({completed_points:.1f} pts)")
lines.append(f"- Created this window: {len(created_this_week)}")
lines.append("")

lines.append("## Completed This Window")
if not completed_this_week:
    lines.append("- None (or missing `resolutiondate` in current export payload)")
else:
    for i in completed_this_week[:20]:
        f = fields(i)
        assignee = (f.get("assignee") or {}).get("displayName", "Unassigned")
        lines.append(f"- {i.get('key')} ({assignee}) {f.get('summary', '')}")
lines.append("")

lines.append("## In Progress Now")
if not in_progress_now:
    lines.append("- None")
else:
    for i in in_progress_now[:20]:
        f = fields(i)
        assignee = (f.get("assignee") or {}).get("displayName", "Unassigned")
        lines.append(f"- {i.get('key')} ({assignee}) {f.get('summary', '')}")
lines.append("")

lines.append("## Next Up (Top 5 Open)")
for i in open_now[:5]:
    f = fields(i)
    due = f.get("duedate") or "-"
    lines.append(f"- {i.get('key')} (due={due}) {f.get('summary', '')}")
if not open_now:
    lines.append("- None")
lines.append("")

lines.append("## Notes")
lines.append("- Weekly completion metrics use Jira `resolutiondate` when available.")
lines.append("- Ensure daily export includes `created/updated/resolutiondate` fields.")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

print(out_path)
PY

RELEASE_VERSION="${3:-}"
CHECK_CMD=(python3 "$MAIN_ROOT/tools/release_sync_check.py" --self-only)
if [[ -n "$RELEASE_VERSION" ]]; then
  CHECK_CMD+=(--version "$RELEASE_VERSION")
fi

set +e
CHECK_OUT="$("${CHECK_CMD[@]}" 2>&1)"
CHECK_RC=$?
set -e

{
  echo "# Weekly Alignment Checklist ($DATE)"
  echo
  echo "## Release / Tag / Jira Alignment"
  if [[ "$CHECK_RC" -eq 0 ]]; then
    echo "- [x] release_sync_check.py passed with no FAIL conditions."
  else
    echo "- [ ] release_sync_check.py reported FAIL conditions (see details below)."
  fi
  if printf "%s\n" "$CHECK_OUT" | grep -q "WARN"; then
    echo "- [ ] Warnings present; review needed."
  else
    echo "- [x] No warnings reported."
  fi
  if printf "%s\n" "$CHECK_OUT" | grep -q "\\[OK\\] main.tag"; then
    echo "- [x] Main release tag signal detected."
  else
    echo "- [ ] Main release tag signal missing."
  fi
  if printf "%s\n" "$CHECK_OUT" | grep -q "\\[OK\\] main.changelog"; then
    echo "- [x] Main changelog heading aligned."
  else
    echo "- [ ] Main changelog heading not aligned."
  fi
  if printf "%s\n" "$CHECK_OUT" | grep -q "\\[OK\\] status.doc: BasaltOS release matches"; then
    echo "- [x] Jira release sync status doc aligned for BasaltOS."
  else
    echo "- [ ] Jira release sync status doc not aligned for BasaltOS."
  fi
  echo
  echo "## Raw Check Output"
  echo
  echo '```text'
  printf "%s\n" "$CHECK_OUT"
  echo '```'
  echo
  echo "## Follow-up"
  echo "- Re-run: \`python3 tools/release_sync_check.py --self-only\` after tag/changelog/status updates."
} > "$ALIGN_FILE"

echo "$SHIPPING_FILE"
echo "$ALIGN_FILE"
echo "Wrote: $SHIPPING_FILE"
echo "Wrote: $ALIGN_FILE"
