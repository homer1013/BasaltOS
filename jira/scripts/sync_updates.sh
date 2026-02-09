#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$ROOT/config/jira.env"

usage() {
  cat <<USAGE
Usage:
  bash jira/scripts/sync_updates.sh [--dry-run] [--archive] <updates.json>

Options:
  --dry-run   Print operations without calling Jira APIs
  --archive   Move applied file to jira/updates/applied/ with timestamp suffix
USAGE
}

DRY_RUN=false
ARCHIVE=false
UPDATE_FILE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=true; shift ;;
    --archive) ARCHIVE=true; shift ;;
    -h|--help) usage; exit 0 ;;
    *) UPDATE_FILE="$1"; shift ;;
  esac
done

if [[ -z "$UPDATE_FILE" ]]; then
  usage
  exit 1
fi

if [[ ! -f "$ENV_FILE" ]]; then
  echo "Missing $ENV_FILE" >&2
  exit 1
fi

if [[ ! -f "$UPDATE_FILE" ]]; then
  echo "Missing updates file: $UPDATE_FILE" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$ENV_FILE"

: "${JIRA_SITE:?}"
: "${JIRA_EMAIL:?}"
: "${JIRA_TOKEN:?}"

log() { printf '[sync] %s\n' "$*"; }

require_issue() {
  local issue="$1"
  local action="$2"
  if [[ -z "$issue" ]]; then
    echo "${action} requires 'issue'" >&2
    exit 1
  fi
}

api_get() {
  local path="$1"
  curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
    -H "Accept: application/json" \
    "$JIRA_SITE$path"
}

api_post() {
  local path="$1"
  local payload="$2"
  curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
    -H "Accept: application/json" -H "Content-Type: application/json" \
    -X POST "$JIRA_SITE$path" \
    -d "$payload"
}

api_put() {
  local path="$1"
  local payload="$2"
  curl --fail-with-body -sS -u "$JIRA_EMAIL:$JIRA_TOKEN" \
    -H "Accept: application/json" -H "Content-Type: application/json" \
    -X PUT "$JIRA_SITE$path" \
    -d "$payload" >/dev/null
}

SELF_ACCOUNT_ID=""
if [[ "$DRY_RUN" == false ]]; then
  SELF_ACCOUNT_ID="$(api_get '/rest/api/3/myself' | python3 -c 'import sys,json; print(json.load(sys.stdin)["accountId"])')"
fi

ops_jsonl="$(python3 - "$UPDATE_FILE" <<'PY'
import json, sys
p = sys.argv[1]
d = json.load(open(p, 'r', encoding='utf-8'))
ops = d.get('operations', [])
if not isinstance(ops, list):
    raise SystemExit('`operations` must be an array')
for i, op in enumerate(ops, 1):
    if not isinstance(op, dict):
        raise SystemExit(f'operation {i} is not an object')
    if 'action' not in op:
        raise SystemExit(f'operation {i} missing action')
    action = str(op['action'])
    if action != 'create_issue' and 'issue' not in op:
        raise SystemExit(f'operation {i} ({action}) missing issue')
    op['_index'] = i
    print(json.dumps(op, ensure_ascii=True))
PY
)"

if [[ -z "$ops_jsonl" ]]; then
  log "No operations found in $UPDATE_FILE"
  exit 0
fi

while IFS= read -r op; do
  [[ -z "$op" ]] && continue

  action="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read())["action"])' <<< "$op")"
  issue="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("issue",""))' <<< "$op")"
  idx="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read())["_index"])' <<< "$op")"

  if [[ -n "$issue" ]]; then
    log "#${idx} ${action} ${issue}"
  else
    log "#${idx} ${action}"
  fi

  case "$action" in
    create_issue)
      project_default="${JIRA_PROJECT_KEY:-}"
      payload="$(python3 - "$op" "$project_default" <<'PY'
import json,sys
op=json.loads(sys.argv[1])
project_default=sys.argv[2]
project_key=op.get('project', project_default)
summary=op.get('summary','').strip()
issue_type=op.get('issue_type','Task')
labels=op.get('labels',[])
parent=op.get('parent')
fields_extra=op.get('fields',{})
desc=op.get('description','').strip()
if not project_key:
    raise SystemExit('create_issue missing project and JIRA_PROJECT_KEY is not set')
if not summary:
    raise SystemExit('create_issue missing summary')
fields={
    'project': {'key': project_key},
    'summary': summary,
    'issuetype': {'name': issue_type},
}
if labels:
    fields['labels']=labels
if parent:
    fields['parent']={'key': parent}
if desc:
    fields['description']={
        'type':'doc','version':1,
        'content':[{'type':'paragraph','content':[{'type':'text','text':desc}]}],
    }
if isinstance(fields_extra, dict):
    fields.update(fields_extra)
print(json.dumps({'fields': fields}))
PY
)"
      summary="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("summary",""))' <<< "$op")"
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN create_issue: $summary"
      else
        resp="$(api_post '/rest/api/3/issue' "$payload")"
        key="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("key",""))' <<< "$resp")"
        [[ -n "$key" ]] || { echo "create_issue did not return key" >&2; exit 1; }
        log "Created issue: $key"
      fi
      ;;

    comment)
      require_issue "$issue" "$action"
      body="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("body",""))' <<< "$op")"
      [[ -n "$body" ]] || { echo "comment missing body for ${issue}" >&2; exit 1; }
      payload="$(python3 -c 'import json,sys; txt=sys.stdin.read(); print(json.dumps({"body":{"type":"doc","version":1,"content":[{"type":"paragraph","content":[{"type":"text","text":txt}]}]}}))' <<< "$body")"
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN comment: $body"
      else
        api_post "/rest/api/3/issue/${issue}/comment" "$payload" >/dev/null
      fi
      ;;

    transition)
      require_issue "$issue" "$action"
      to="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("to",""))' <<< "$op")"
      [[ -n "$to" ]] || { echo "transition missing 'to' for ${issue}" >&2; exit 1; }
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN transition to: $to"
      else
        tid="$(api_get "/rest/api/3/issue/${issue}/transitions" | python3 -c 'import json,sys; d=json.load(sys.stdin); target=sys.argv[1].strip().lower()
for t in d.get("transitions",[]):
    if t.get("name","").strip().lower()==target:
        print(t["id"]); raise SystemExit(0)
raise SystemExit("transition not found")' "$to")"
        api_post "/rest/api/3/issue/${issue}/transitions" "{\"transition\":{\"id\":\"${tid}\"}}" >/dev/null
      fi
      ;;

    transition_id)
      require_issue "$issue" "$action"
      tid="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("id",""))' <<< "$op")"
      [[ -n "$tid" ]] || { echo "transition_id missing 'id' for ${issue}" >&2; exit 1; }
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN transition id: $tid"
      else
        api_post "/rest/api/3/issue/${issue}/transitions" "{\"transition\":{\"id\":\"${tid}\"}}" >/dev/null
      fi
      ;;

    assign_self)
      require_issue "$issue" "$action"
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN assign self"
      else
        api_put "/rest/api/3/issue/${issue}/assignee" "{\"accountId\":\"${SELF_ACCOUNT_ID}\"}"
      fi
      ;;

    assign)
      require_issue "$issue" "$action"
      account_id="$(python3 -c 'import json,sys; print(json.loads(sys.stdin.read()).get("accountId",""))' <<< "$op")"
      [[ -n "$account_id" ]] || { echo "assign missing accountId for ${issue}" >&2; exit 1; }
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN assign accountId: $account_id"
      else
        api_put "/rest/api/3/issue/${issue}/assignee" "{\"accountId\":\"${account_id}\"}"
      fi
      ;;

    set_fields)
      require_issue "$issue" "$action"
      fields_payload="$(python3 -c 'import json,sys; d=json.loads(sys.stdin.read()); print(json.dumps({"fields": d.get("fields", {})}))' <<< "$op")"
      if [[ "$DRY_RUN" == true ]]; then
        log "DRY-RUN set_fields: $fields_payload"
      else
        api_put "/rest/api/3/issue/${issue}" "$fields_payload"
      fi
      ;;

    *)
      echo "Unsupported action: $action" >&2
      exit 1
      ;;
  esac

done <<< "$ops_jsonl"

log "Applied operations from $UPDATE_FILE"

if [[ "$ARCHIVE" == true ]]; then
  mkdir -p "$ROOT/updates/applied"
  ts="$(date +%Y%m%d_%H%M%S)"
  base="$(basename "$UPDATE_FILE")"
  mv "$UPDATE_FILE" "$ROOT/updates/applied/${ts}_${base}"
  log "Archived to jira/updates/applied/${ts}_${base}"
fi
