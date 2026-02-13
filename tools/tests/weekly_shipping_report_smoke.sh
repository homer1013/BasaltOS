#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TMP="tmp/weekly_shipping_report_smoke"
EXPORT_DIR="$TMP/export"
REPORT_DIR="$ROOT/jira/reports"
DATE="$(date +%F)"
SHIP="$REPORT_DIR/weekly_shipping_${DATE}.md"
ALIGN="$REPORT_DIR/weekly_alignment_${DATE}.md"

rm -rf "$TMP"
mkdir -p "$EXPORT_DIR"

cat > "$EXPORT_DIR/issues.json" <<'JSON'
{
  "issues": [
    {
      "key": "SCRUM-900",
      "fields": {
        "summary": "Completed sample issue",
        "status": {"name": "Done"},
        "labels": ["release:v0.1.0"],
        "assignee": {"displayName": "Homer Morrill"},
        "priority": {"name": "High"},
        "customfield_10016": 2,
        "created": "2026-02-12T11:00:00.000+0000",
        "resolutiondate": "2026-02-13T10:00:00.000+0000"
      }
    },
    {
      "key": "SCRUM-901",
      "fields": {
        "summary": "In progress sample issue",
        "status": {"name": "In Progress"},
        "labels": ["release:v0.1.0"],
        "assignee": {"displayName": "Homer Morrill"},
        "priority": {"name": "Medium"},
        "customfield_10016": 1,
        "created": "2026-02-13T12:00:00.000+0000"
      }
    }
  ]
}
JSON

bash jira/scripts/report_weekly_shipping.sh "$EXPORT_DIR" 7 >/tmp/weekly_shipping_report_smoke.out

test -f "$SHIP" || { echo "FAIL: missing $SHIP"; exit 1; }
test -f "$ALIGN" || { echo "FAIL: missing $ALIGN"; exit 1; }

grep -q "Weekly Shipping Summary" "$SHIP"
grep -q "Completed This Window" "$SHIP"
grep -q "SCRUM-900" "$SHIP"
grep -q "Weekly Alignment Checklist" "$ALIGN"
grep -q "Raw Check Output" "$ALIGN"

echo "PASS: weekly shipping report smoke"
