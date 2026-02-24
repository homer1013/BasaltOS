# Release Sync Status

Current cross-repo release alignment snapshot.

| Repo | Release | Status | Notes |
| --- | --- | --- | --- |
| BasaltOS | v0.1.2-rc4-configurator-m5-bench-20260224 | synced | Main tag/release published |
| BasaltOS_Platform | v0.1.2-rc4-configurator-m5-bench-20260224 | in_progress | Pending platform sync follow-up |
| BasaltOS_PlatformIO | v0.1.2-rc4-configurator-m5-bench-20260224 | in_progress | Pending platformio sync follow-up |

## Update Rule

When `BasaltOS_Main` release tag changes:
- update all 3 rows to the new release value,
- change status as needed during sync (`in_progress` -> `synced`),
- run `python tools/release_sync_check.py --version <new-tag>`.
