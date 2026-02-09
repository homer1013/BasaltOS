# Jira Workspace

Local Jira planning + daily JSON exports + structured sync updates.

## Layout
- `config/` (`jira.env` is local and gitignored)
- `scripts/` (`export_daily.sh`, `sync_updates.sh`)
- `updates/` (`pending/`, `applied/`, templates)
- `exports/` (gitignored snapshots)
- `logs/` (gitignored logs)
- `reports/` (generated markdown, keep sanitized before commit)

## Daily loop

Morning:
1. `bash jira/scripts/export_daily.sh`
2. Share latest export JSON with Codex for planning.

During day:
1. Add/update `jira/updates/pending/*.json`
2. Dry-run: `bash jira/scripts/sync_updates.sh --dry-run jira/updates/pending/<file>.json`
3. Apply: `bash jira/scripts/sync_updates.sh --archive jira/updates/pending/<file>.json`

End of day:
1. Run sync updates for final status/comments/transitions.
2. `bash jira/scripts/export_daily.sh` to snapshot state.

## Privacy
- Do not commit `jira/config/jira.env`.
- Do not commit raw `jira/exports/*` or `jira/logs/*`.
- Keep `jira/updates/applied/*` and `jira/reports/*` generic/sanitized if shared publicly.

Blocker report:

```bash
bash jira/scripts/report_blockers.sh
```

This writes a release blocker snapshot to `jira/reports/`.

Sprint review/carryover report:

```bash
bash jira/scripts/report_sprint_review.sh
```

This writes sprint review and carryover candidate markdown files to `jira/reports/`.

Release checklist snapshot from latest export:

```bash
bash jira/scripts/report_release_checklist.sh
```

This writes a release checklist markdown snapshot to `jira/reports/`.

Daily status digest + social draft from latest export:

```bash
bash jira/scripts/report_daily_status.sh
```

This writes `daily_status_*.md` and `daily_social_*.md` to `jira/reports/`.


Sprint cadence policy:

- See `jira/docs/SPRINT_OPERATING_MODEL.md` for the single active 2-week sprint rules.
- Use `jira/updates/template.mid_sprint_add.json` for controlled mid-sprint intake.

Rough-edge burn-down report (CLI/configurator):

```bash
bash jira/scripts/report_rough_edges.sh
```

This writes a top-5 focus report to `jira/reports/`.
