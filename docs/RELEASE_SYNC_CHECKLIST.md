# Release Sync Checklist (Main -> Platform/PlatformIO)

Use this checklist whenever a new `BasaltOS_Main` tag is created.


## 0) Preflight guard (required)
- Update `docs/RELEASE_SYNC_STATUS.md` target release rows.
- Run checker in main repo:
  - `python3 tools/release_sync_check.py --version <tag>`
- CI also runs self-check via `tools/tests/release_sync_check_smoke.sh`.

## 1) Main repo release
- Create and push tag (example: `v0.2.0`).
- Publish GitHub release notes including contract/schema changes.
- Confirm CI green at tagged commit.

## 2) Platform sync
- Open issue in `BasaltOS_Platform` to consume new tag.
- Update references/docs to new contract version.
- Validate platform pages still align with scope and wording.

## 3) PlatformIO sync
- Open issue in `BasaltOS_PlatformIO` to consume new tag.
- Re-run bootstrap + contract smoke checks.
- Update integration docs/extension notes for any changed inputs.

## 4) Cross-repo closure
- Link platform and platformio sync PR/issues back to main release.
- Mark sync complete in all 3 repos.
