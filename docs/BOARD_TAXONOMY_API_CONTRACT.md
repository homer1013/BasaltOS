# Board Taxonomy API Contract (v1)

This document defines concrete request/response examples for board taxonomy integration in the local configurator backend.

Base URL (local):

- `http://127.0.0.1:5000`

## Endpoint: `GET /api/board-taxonomy`

Purpose:

- Return machine-readable board taxonomy index rows.
- Support optional exact-match filters for platform taxonomy dimensions.

Supported query params:

- `platform`
- `manufacturer`
- `architecture`
- `family`

Example request:

```bash
curl --fail-with-body -sS \
  "http://127.0.0.1:5000/api/board-taxonomy?platform=esp32&architecture=risc-v"
```

Example response shape:

```json
{
  "success": true,
  "filters": {
    "platform": "esp32",
    "manufacturer": "",
    "architecture": "risc-v",
    "family": ""
  },
  "summary": {
    "total_boards": 2,
    "platform_count": 1,
    "manufacturer_count": 1,
    "architecture_count": 1,
    "family_count": 1
  },
  "counts": {
    "platform": { "esp32": 2 },
    "manufacturer": { "Espressif": 2 },
    "architecture": { "RISC-V": 2 },
    "family": { "ESP32-C3": 2 }
  },
  "boards": [
    {
      "platform": "esp32",
      "board_dir": "esp32-c3-supermini",
      "id": "esp32-c3-supermini",
      "name": "ESP32-C3 SuperMini",
      "manufacturer": "Espressif",
      "architecture": "RISC-V",
      "family": "ESP32-C3",
      "mcu": "ESP32-C3"
    }
  ]
}
```

Response notes:

- `success` is always present (`true` on success).
- `filters` echoes normalized filter values.
- `counts` is scoped to filtered results.
- `boards` rows include canonical taxonomy fields used by picker/search flows.

## Endpoint: `GET /api/boards/<platform>/options`

Purpose:

- Provide cascading board-picker options and counts for one platform.
- Supports incremental filtering by upstream selections and text search.

Path param:

- `<platform>` (example: `esp32`)

Supported query params:

- `manufacturer`
- `architecture`
- `family`
- `silicon` (matches taxonomy `mcu`)
- `q` (free-text search across id/dir/name/manufacturer/architecture/family/mcu)

Example request:

```bash
curl --fail-with-body -sS \
  "http://127.0.0.1:5000/api/boards/esp32/options?manufacturer=Espressif&architecture=RISC-V&q=c3"
```

Example response shape:

```json
{
  "success": true,
  "filters": {
    "platform": "esp32",
    "manufacturer": "Espressif",
    "architecture": "RISC-V",
    "family": "",
    "silicon": "",
    "q": "c3"
  },
  "summary": {
    "total_boards": 2,
    "options_count": {
      "manufacturer": 1,
      "architecture": 1,
      "family": 1,
      "silicon": 1
    }
  },
  "counts": {
    "manufacturer": { "Espressif": 2 },
    "architecture": { "RISC-V": 2 },
    "family": { "ESP32-C3": 2 },
    "silicon": { "ESP32-C3": 2 }
  },
  "options": {
    "manufacturer": ["Espressif"],
    "architecture": ["RISC-V"],
    "family": ["ESP32-C3"],
    "silicon": ["ESP32-C3"]
  }
}
```

Response notes:

- `filters.platform` is normalized lowercase.
- If a requested filter value is not present in current domain options, backend resets that filter to empty in the response.
- `counts` and `options` represent post-filter cascading domains for picker steps.

## Integration Notes

- Client should treat missing/empty options as valid states (not hard errors).
- Cascading flow recommendation:
  - Call `/api/boards/<platform>/options` after each picker change.
  - Use returned `filters` as source of truth for normalized current state.
  - Use returned `options` and `counts` to render next domain choices.
- Use `/api/board-taxonomy` for inventory/reporting/search-index use cases.
- Use `/api/boards/<platform>/options` for interactive picker UX state.
