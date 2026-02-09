# Release Sync Status

Current cross-repo release alignment snapshot.

| Repo | Release | Status | Notes |
| --- | --- | --- | --- |
| BasaltOS | v0.1.0 | synced | Main tag/release baseline |
| BasaltOS_Platform | v0.1.0 | synced | Governance + release sync docs aligned |
| BasaltOS_PlatformIO | v0.1.0 | synced | Bootstrap CI and release sync docs aligned |

## Update Rule

When `BasaltOS_Main` release tag changes:
- update all 3 rows to the new release value,
- change status as needed during sync (`in_progress` -> `synced`),
- run `python tools/release_sync_check.py --version <new-tag>`.
