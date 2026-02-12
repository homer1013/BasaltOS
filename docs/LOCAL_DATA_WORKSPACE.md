# BasaltOS Local Data Workspace Contract

This document defines where local-only BasaltOS data should live in a cloned repo.

## Goal

Keep source code and reproducible artifacts in git, while keeping user-owned local
data and machine-specific caches out of git.

## Canonical Local Data Root

- Default root: `.basaltos-local/` (repo-local)
- Optional override: `BASALTOS_LOCAL_DATA_DIR`

If `BASALTOS_LOCAL_DATA_DIR` is relative, it is resolved from repo root.

## Layout

Suggested structure under local data root:

- `cache/builds/`
  - machine/toolchain build outputs
  - disposable, can be rebuilt
- `user/configs/`
  - saved configuration presets and snapshots
- `user/boards/`
  - local board inventory and custom board metadata
- `user/exports/`
  - user-triggered export bundles for backup/sync
- `reports/`
  - migration/status reports

## Git Policy

- Local data root is git-ignored (`.basaltos-local/`).
- Build outputs in repo root (`build/`, `build_*`) are also git-ignored.
- Local data is never committed by default.

## Guest-First Model

- Guest users can save local data with no account.
- Local data remains available offline.
- No forced cloud sync.

## Optional Account Sync (Future Platform Work)

When users later choose to connect an account on BasaltOS Platform:

1. Local data stays local by default.
2. User explicitly chooses what to sync.
3. Sync uses export/import payloads with schema versioning.
4. Conflict behavior is explicit (prompt/merge strategy), not silent overwrite.

## Migration Helper

Use:

```bash
python3 tools/local_data_migrate.py --dry-run
python3 tools/local_data_migrate.py --apply
```

This helper can migrate common ad-hoc local folders (for example `build_*`) into
the canonical local data root and generate a report.
