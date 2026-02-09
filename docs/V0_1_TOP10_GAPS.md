# BasaltOS v0.1.0 Top-10 Gap List

Generated from Sprint 1 baseline execution evidence.

| Rank | Jira | Severity | Target Sprint | Owner | Gap |
|---|---|---|---|---|---|
| 1 | SCRUM-52 | Critical | Sprint 1 | Homer Morrill | CLI: invalid board does not fail (returns 0 and generates config) |
| 2 | SCRUM-53 | High | Sprint 2 | Homer Morrill | CLI: non-ESP boards still print ESP-IDF next steps |
| 3 | SCRUM-54 | High | Sprint 2 | Homer Morrill | Local configurator docs still describe platform/product features out of local-tool scope |
| 4 | SCRUM-55 | Medium | Sprint 3 | Homer Morrill | Local configurator includes marketplace/profile surface area beyond dev-tool scope |
| 5 | SCRUM-56 | High | Sprint 2 | Homer Morrill | No automated regression test for invalid-board hard-fail path in configure.py |
| 6 | SCRUM-57 | High | Sprint 2 | Homer Morrill | No CI gate for configure.py smoke across ESP32/STM32/RP2040/AVR |
| 7 | SCRUM-58 | Medium | Sprint 2 | Homer Morrill | No deterministic-output diff check for generated config artifacts |
| 8 | SCRUM-59 | Medium | Sprint 3 | Homer Morrill | Configurator API smoke/deep checks are not wired into CI |
| 9 | SCRUM-60 | Medium | Sprint 1 | Homer Morrill | Release readiness lacks scripted blocker report generation |
| 10 | SCRUM-61 | Low | Sprint 1 | Homer Morrill | Sprint review/carryover process is not yet script-assisted |

## Evidence Sources

- `tmp/acceptance/v0.1.0/2026-02-08/commands.log`
- `tmp/acceptance/v0.1.0/2026-02-08/artifacts/`
- `docs/V0_1_ACCEPTANCE_CHECKLIST.md`
