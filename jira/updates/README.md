# Jira Updates

This folder stores structured Jira mutations that can be applied by
`jira/scripts/sync_updates.sh`.

## Flow
1. Create a JSON file under `jira/updates/pending/`.
2. Dry-run it:
   bash jira/scripts/sync_updates.sh --dry-run jira/updates/pending/<file>.json
3. Apply + archive it:
   bash jira/scripts/sync_updates.sh --archive jira/updates/pending/<file>.json

## Supported actions
- comment
- transition
- transition_id
- assign_self
- assign
- set_fields
- create_issue
