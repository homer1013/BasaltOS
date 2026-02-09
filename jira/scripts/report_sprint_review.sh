#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$ROOT/config/jira.env"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Missing $ENV_FILE (copy from jira.env.example first)" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$ENV_FILE"

: "${JIRA_SITE:?}"
: "${JIRA_EMAIL:?}"
: "${JIRA_TOKEN:?}"
: "${JIRA_PROJECT_KEY:?}"
: "${JIRA_BOARD_ID:?}"

DATE="$(date +%F)"
OUT_DIR="$ROOT/reports"
mkdir -p "$OUT_DIR"

SPRINT_JSON="$(mktemp)"
ISSUES_JSON="$(mktemp)"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
  -H "Accept: application/json" \
  "$JIRA_SITE/rest/agile/1.0/board/$JIRA_BOARD_ID/sprint?maxResults=200" > "$SPRINT_JSON"

ACTIVE_SPRINT_ID="$(python3 - "$SPRINT_JSON" <<'PY'
import json,sys
s=json.load(open(sys.argv[1]))
vals=s.get('values',[])
active=[x for x in vals if x.get('state')=='active']
print(active[0]['id'] if active else '')
PY
)"

if [[ -z "$ACTIVE_SPRINT_ID" ]]; then
  echo "No active sprint found for board $JIRA_BOARD_ID" >&2
  rm -f "$SPRINT_JSON" "$ISSUES_JSON"
  exit 1
fi

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
  -H "Accept: application/json" \
  "$JIRA_SITE/rest/agile/1.0/sprint/$ACTIVE_SPRINT_ID/issue?maxResults=1000" > "$ISSUES_JSON"

python3 - "$SPRINT_JSON" "$ISSUES_JSON" "$OUT_DIR" "$DATE" <<'PY'
import json,sys,re
from pathlib import Path

sprint_json, issues_json, out_dir, date_str = sys.argv[1:]
sprint_data=json.load(open(sprint_json,'r',encoding='utf-8'))
issue_data=json.load(open(issues_json,'r',encoding='utf-8'))

active=[x for x in sprint_data.get('values',[]) if x.get('state')=='active'][0]
future=[x for x in sprint_data.get('values',[]) if x.get('state')=='future']
next_sprint=future[0] if future else None

issues=issue_data.get('issues',[])
work=[i for i in issues if i['fields']['issuetype']['name']!='Epic']
completed=[i for i in work if i['fields']['status']['name']=='Done']
open_items=[i for i in work if i['fields']['status']['name']!='Done']

pts=lambda i: i['fields'].get('customfield_10016') or 0
committed_points=sum(float(pts(i)) for i in work if isinstance(pts(i),(int,float)))
completed_points=sum(float(pts(i)) for i in completed if isinstance(pts(i),(int,float)))

safe_name=re.sub(r'[^A-Za-z0-9_-]+','_',active.get('name','sprint')).strip('_')
report_path=Path(out_dir)/f"sprint_review_{safe_name}_{date_str}.md"
carry_path=Path(out_dir)/f"carryover_candidates_{safe_name}_{date_str}.md"

lines=[]
lines.append(f"# Sprint Review ({active.get('name')}) - {date_str}")
lines.append('')
lines.append('## Summary')
lines.append(f"- Sprint id: `{active.get('id')}`")
lines.append(f"- Sprint goal: {active.get('goal') or '(none)'}")
lines.append(f"- Start: {active.get('startDate') or '-'}")
lines.append(f"- End: {active.get('endDate') or '-'}")
lines.append('')
lines.append('## Metrics')
lines.append(f"- Committed work items (non-epic): {len(work)}")
lines.append(f"- Completed work items: {len(completed)}")
lines.append(f"- Open work items: {len(open_items)}")
lines.append(f"- Story points committed: {committed_points:.1f}")
lines.append(f"- Story points completed: {completed_points:.1f}")
lines.append('')
lines.append('## Open Items')
if not open_items:
    lines.append('- None')
else:
    for i in sorted(open_items,key=lambda x:int(x['key'].split('-')[1])):
      f=i['fields']
      ass=(f.get('assignee') or {}).get('displayName') or 'Unassigned'
      lines.append(f"- {i['key']} [{f['status']['name']}] ({f['issuetype']['name']}, assignee={ass}) {f['summary']}")

report_path.write_text('\n'.join(lines)+'\n', encoding='utf-8')

cl=[]
cl.append(f"# Carryover Candidates ({active.get('name')}) - {date_str}")
cl.append('')
if next_sprint:
    cl.append(f"Next sprint detected: `{next_sprint.get('name')}` (id `{next_sprint.get('id')}`)")
else:
    cl.append('No future sprint detected. Create next sprint before moving carryover items.')
cl.append('')
cl.append('## Candidates')
if not open_items:
    cl.append('- None')
else:
    for i in sorted(open_items,key=lambda x:int(x['key'].split('-')[1])):
        cl.append(f"- {i['key']} {i['fields']['summary']}")
cl.append('')
cl.append('## Move Commands')
if open_items and next_sprint:
    keys=[i['key'] for i in open_items]
    key_json=', '.join([f'\"{k}\"' for k in keys])
    cl.append('```bash')
    cl.append('source jira/config/jira.env')
    cl.append('curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \\')
    cl.append('  -H "Accept: application/json" -H "Content-Type: application/json" \\')
    cl.append(f'  -X POST "$JIRA_SITE/rest/agile/1.0/sprint/{next_sprint.get("id")}/issue" \\')
    cl.append(f'  -d "{{\"issues\":[{key_json}]}}"')
    cl.append('```')
else:
    cl.append('- No move command generated.')

carry_path.write_text('\n'.join(cl)+'\n', encoding='utf-8')
print(report_path)
print(carry_path)
PY

rm -f "$SPRINT_JSON" "$ISSUES_JSON"
