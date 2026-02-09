# Sprint Operating Model (Solo, 2-Week Cadence)

## Goal
Use one active 2-week sprint at a time, avoid micro-sprints, and add work only when capacity allows.

## Rules
- Keep exactly one active sprint.
- Keep WIP low: max 1 issue `In Progress`, max 1 issue `In Review`.
- Add work from backlog intentionally, not ad hoc.
- Label any issue added after sprint start with `added-mid-sprint`.
- Keep `cadence:2wk` on sprint-managed issues for quick filtering.

## Daily Routine
1. Export snapshot: `bash jira/scripts/export_daily.sh`
2. Do implementation work.
3. Apply Jira updates from `jira/updates/pending/*.json`.
4. Re-export and generate reports:
   - `bash jira/scripts/report_daily_status.sh`
   - `bash jira/scripts/report_release_checklist.sh`

## End-of-Sprint Routine
1. Finish/transition what is done.
2. Generate sprint review + carryover:
   - `bash jira/scripts/report_sprint_review.sh`
3. Publish retrospective notes.
4. Start next 2-week sprint and pull carryover items in first.
