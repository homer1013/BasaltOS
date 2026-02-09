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

DATE="$(date +%F)"
OUT_DIR="$ROOT/reports"
mkdir -p "$OUT_DIR"
OUT_FILE="$OUT_DIR/v010_blockers_${DATE}.md"
TMP_JSON="$(mktemp)"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
  -H "Accept: application/json" \
  --get "$JIRA_SITE/rest/api/3/search/jql" \
  --data-urlencode "jql=project=$JIRA_PROJECT_KEY AND labels = \"release:v0.1.0\" ORDER BY priority DESC, status, key ASC" \
  --data-urlencode "maxResults=1000" \
  --data-urlencode "fields=summary,status,issuetype,labels,assignee,priority,duedate,parent,customfield_10016" \
  > "$TMP_JSON"

python3 - "$TMP_JSON" "$OUT_FILE" <<'PY'
import json, sys, datetime
inp, outp = sys.argv[1], sys.argv[2]
d = json.load(open(inp, 'r', encoding='utf-8'))
issues = d.get('issues', [])

open_items = [i for i in issues if i['fields']['status']['name'] != 'Done']
done_items = [i for i in issues if i['fields']['status']['name'] == 'Done']

by_area = {"area:cli":0, "area:configurator":0, "area:metadata":0, "area:docs":0, "other":0}
for i in open_items:
    labels = set(i['fields'].get('labels') or [])
    matched = False
    for a in ["area:cli","area:configurator","area:metadata","area:docs"]:
        if a in labels:
            by_area[a] += 1
            matched = True
    if not matched:
        by_area["other"] += 1

pts = lambda i: i['fields'].get('customfield_10016') or 0
open_points = sum(float(pts(i)) for i in open_items if isinstance(pts(i),(int,float)))
done_points = sum(float(pts(i)) for i in done_items if isinstance(pts(i),(int,float)))

lines=[]
lines.append(f"# v0.1.0 Blocker Report ({datetime.date.today().isoformat()})")
lines.append("")
lines.append("## Summary")
lines.append(f"- Total release-tagged issues: {len(issues)}")
lines.append(f"- Open issues: {len(open_items)}")
lines.append(f"- Done issues: {len(done_items)}")
lines.append(f"- Open story points: {open_points:.1f}")
lines.append(f"- Done story points: {done_points:.1f}")
lines.append("")
lines.append("## Open By Area")
for k,v in by_area.items():
    lines.append(f"- {k}: {v}")

lines.append("")
lines.append("## Open Issues")
if not open_items:
    lines.append("- None")
else:
    for i in open_items:
        f=i['fields']
        key=i['key']
        pri=(f.get('priority') or {}).get('name','')
        ass=(f.get('assignee') or {}).get('displayName','Unassigned')
        due=f.get('duedate') or '-'
        lines.append(f"- {key} [{f['status']['name']}] ({f['issuetype']['name']}, priority={pri}, assignee={ass}, due={due}) {f['summary']}")

lines.append("")
lines.append("## Top Priority Open")
prio_order={"Highest":5,"High":4,"Medium":3,"Low":2,"Lowest":1}
open_sorted=sorted(open_items, key=lambda i:(-prio_order.get((i['fields'].get('priority') or {}).get('name','Medium'),3), i['key']))
for i in open_sorted[:10]:
    f=i['fields']
    pri=(f.get('priority') or {}).get('name','')
    lines.append(f"- {i['key']} ({pri}) {f['summary']}")

open(outp,'w',encoding='utf-8').write('\n'.join(lines)+'\n')
print(outp)
PY

rm -f "$TMP_JSON"

echo "Wrote: $OUT_FILE"
