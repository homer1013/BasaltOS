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
OUT_DIR="$ROOT/exports/$DATE"
LOG_DIR="$ROOT/logs"
mkdir -p "$OUT_DIR" "$LOG_DIR"

log="$LOG_DIR/export_${DATE}.log"
exec > >(tee -a "$log") 2>&1

echo "Exporting Jira snapshot for $DATE"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" -H "Accept: application/json" \
  "$JIRA_SITE/rest/api/3/project/$JIRA_PROJECT_KEY" > "$OUT_DIR/project.json"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" -H "Accept: application/json" \
  "$JIRA_SITE/rest/api/3/field" > "$OUT_DIR/fields.json"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" -H "Accept: application/json" \
  "$JIRA_SITE/rest/agile/1.0/board?maxResults=50" > "$OUT_DIR/boards.json"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" -H "Accept: application/json" \
  "$JIRA_SITE/rest/agile/1.0/board/$JIRA_BOARD_ID/sprint?maxResults=100" > "$OUT_DIR/sprints_board${JIRA_BOARD_ID}.json"

curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" -H "Accept: application/json" \
  --get "$JIRA_SITE/rest/api/3/search/jql" \
  --data-urlencode "jql=project=$JIRA_PROJECT_KEY ORDER BY key ASC" \
  --data-urlencode "maxResults=500" \
  --data-urlencode "fields=summary,issuetype,status,labels,assignee,parent,priority,duedate,customfield_10016,customfield_10020" \
  > "$OUT_DIR/issues.json"

tar -czf "$ROOT/exports/jira_export_${DATE}.tar.gz" -C "$ROOT/exports" "$DATE"
echo "Done: $OUT_DIR"
echo "Archive: $ROOT/exports/jira_export_${DATE}.tar.gz"
