# Guest-to-Account Sync Contract (Main -> Platform)

This document defines the contract for optional sync of local BasaltOS data from
guest mode (local-only) to account mode (cloud-backed).

Status:
- Local guest-first storage contract exists in `docs/LOCAL_DATA_WORKSPACE.md`.
- This document defines the payload + behavior handoff for Platform integration.

## Principles

1. Guest mode works fully offline with no account.
2. Sync is explicit opt-in only.
3. Local data is never deleted automatically by first sync.
4. Conflicts are visible and resolved with deterministic rules.
5. Payloads are versioned for forward migration.

## Data Classes

- `config_snapshot`
  - Generated config state and selected board/driver/app data.
- `board_inventory`
  - User-owned board records and custom board metadata references.
- `user_preset`
  - Reusable template selections, defaults, and tags.

## Sync Envelope

All exports/imports use the same envelope:

```json
{
  "schema_version": "1.0.0",
  "source": {
    "kind": "basaltos_main_local",
    "app_version": "v0.1.0",
    "device_id": "local-machine-fingerprint"
  },
  "exported_utc": "2026-02-12T08:00:00Z",
  "items": []
}
```

## Item Shape

Each item in `items`:

```json
{
  "item_type": "config_snapshot",
  "item_id": "cfg_abc123",
  "updated_utc": "2026-02-12T07:55:00Z",
  "content_hash": "sha256:...",
  "content": {}
}
```

## First Sync Flow (Guest -> Account)

1. User signs in on Platform and links local workspace.
2. Main exports local envelope.
3. Platform compares `item_id + content_hash` with cloud set.
4. Platform presents sync preview:
   - new local items
   - changed items
   - potential conflicts
5. User chooses apply policy:
   - `local_preferred`
   - `cloud_preferred`
   - `manual_per_item`
6. Platform imports selected items and returns result report.
7. Main stores report under local workspace (`user/exports/` + `reports/`).

## Conflict Rules

- Conflict key: same `item_type` + same `item_id` + different `content_hash`.
- If policy is:
  - `local_preferred`: local replaces cloud, cloud previous version archived.
  - `cloud_preferred`: cloud kept, local copy archived with suffix.
  - `manual_per_item`: user decision required before apply.
- No silent overwrite.

## Error Handling

- Reject unknown `schema_version` with actionable migration hint.
- Partial failure returns per-item status list, not all-or-nothing silence.
- Always return summary counts:
  - `created`
  - `updated`
  - `skipped`
  - `conflicted`
  - `failed`

## Security/Privacy Baseline

- No automatic background sync in v0.1.x.
- User confirmation required for every upload/import operation.
- Payloads contain config metadata, not secrets by default.
- Future phases may add encrypted-at-rest export bundles.

## Non-goals (v0.1.x)

- Real-time bidirectional sync.
- Team collaboration/merge editing.
- Background daemon watchers.

## Handoff to Platform Repo

Platform implementation should provide:

1. Upload endpoint for sync envelope.
2. Dry-run diff endpoint returning conflict preview.
3. Apply endpoint with policy selection.
4. Result report payload compatible with local report storage.
