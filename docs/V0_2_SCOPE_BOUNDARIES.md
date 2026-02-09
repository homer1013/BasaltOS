# BasaltOS v0.2.0 Scope Boundaries (Main Repo, Platform, Extension)

## Purpose
Define clear ownership between the three BasaltOS tracks so execution stays focused and features do not overlap.

## Tracks
- `BasaltOS` main repo: embedded core + local developer tooling.
- `BasaltOS Platform` repo: hosted web/product experience.
- `BasaltOS Extension` repo: VS Code/PlatformIO integration layer.

## 1) BasaltOS Main Repo (source-of-truth)
Owns:
- Core runtime, shell, drivers, metadata, and build/generation tooling.
- Local CLI and local configurator behavior.
- Deterministic config outputs and acceptance tests.
- Hardware capability model (`boards/**`, `modules/**`) and validators.

Must provide stable contracts:
- CLI commands and flags for non-interactive automation.
- Generated outputs:
  - `config/generated/basalt_config.h`
  - `config/generated/sdkconfig.defaults`
  - `config/generated/basalt.features.json`
- Clear machine-readable failure messages for invalid configurations.

Out of scope:
- User accounts, billing, cloud projects, hosted telemetry, social/profile systems.

## 2) BasaltOS Platform Repo (hosted product)
Owns:
- Public website and product pages.
- Account/project management, hosted catalogs, marketplace backend.
- Release visibility and user-facing adoption flows.

Consumes from main repo:
- Versioned metadata snapshots and release artifacts.
- Published docs and compatibility matrices.

Out of scope:
- Defining hardware truth independently from main repo.
- Forking CLI behavior without upstream alignment.

## 3) BasaltOS Extension Repo (VS Code/PlatformIO)
Owns:
- IDE UX for board selection, config generation, build/flash orchestration.
- PlatformIO-centric flow that wraps stable CLI contracts.
- Editor diagnostics and convenience actions.

Consumes from main repo:
- CLI entry points and generated config contracts.
- Metadata schema/version and validation behavior.

Out of scope:
- Reimplementing configurator logic independently.
- Becoming a separate build system with divergent config semantics.

## Integration Contracts (required for all tracks)
- Contract A: CLI automation
  - Commands must support headless invocation and deterministic outputs.
  - Non-zero exit codes on failure with actionable stderr.
- Contract B: Generated artifact schema
  - `basalt.features.json` remains backward-compatible within minor line.
  - Breaking schema changes require release note + migration note.
- Contract C: Metadata versioning
  - Schema changes are versioned and validated in CI.
  - Platform/extension consume tagged metadata revisions.
- Contract D: Release linkage
  - Platform and extension target tagged main-repo releases (not floating assumptions).

## Delivery Priority (now)
1. Keep main repo reliability and onboarding strong.
2. Start Platform MVP with read-only, metadata-driven views.
3. Start extension as a thin wrapper over stable CLI contracts.

## Decision Rules
- If a feature changes hardware/runtime truth -> main repo.
- If a feature changes hosted user/product experience -> platform repo.
- If a feature changes developer IDE workflow only -> extension repo.
- If ambiguous, define contract first, then implement in the owning track.
