# BasaltOS Roadmap (2026 H1)

## Scope and guardrails

This roadmap follows the architecture split defined in your planning docs:

- `BasaltOS` repo = core OS + developer tooling.
- Future `basaltos.io` = product platform (accounts, marketplace backend, cloud features).
- Local configurator and CLI stay focused on reliability, speed, and offline use.

## Success criteria for this roadmap

By the end of this plan, BasaltOS should have:

1. A stable `v0.1.0` developer experience (CLI + local web configurator + metadata validation).
2. PlatformIO integration available as a separate deliverable (`phase 2` outcome).
3. Measurable quality gates (tests, validation, docs, release checklist).
4. Clear separation between repo tooling vs future platform features.

## Release cadence

- Sprint length: 2 weeks
- Planning horizon: 6 sprints (12 weeks)
- Status checkpoints: end of each sprint

## Workstreams

1. Core DX tooling: `tools/configure.py`, `tools/basaltos_config_server.py`, templates, generation outputs.
2. Metadata and compatibility: `boards/**/*.json`, `modules/**/module.json`, schema validation and docs.
3. Reliability and QA: smoke/e2e, deterministic generation, release checks.
4. Ecosystem expansion: PlatformIO support (first as separate integration output).
5. Documentation and onboarding: quickstart, board onboarding, app packaging/validation.

## 12-week plan

### Sprint 1 (Weeks 1-2): Baseline and hardening plan

Goals:
- Freeze current behavior and define `v0.1.0` acceptance targets.
- Identify all configurator and CLI regressions.

Deliverables:
- Roadmap accepted and tracked in repo (`docs/ROADMAP_2026_H1.md`).
- `v0.1.0` acceptance checklist added (CLI, local web, generated files).
- Gap list created for configurator + CLI UX failures.
- Metadata validation run documented for all boards/modules.

Definition of done:
- Team can run a single documented command sequence and reproduce baseline results.
- Top 10 known issues prioritized by severity and owner.

### Sprint 2 (Weeks 3-4): CLI wizard reliability (`v0.1.0` gate 1)

Execution artifact: `docs/S2_CLI_RELIABILITY_MATRIX.md`

Goals:
- Make CLI wizard the most reliable configuration path.
- Ensure generated output consistency.

Deliverables:
- Improved error messages and input validation in `tools/configure.py`.
- Stable generation of:
  - `config/generated/basalt_config.h`
  - `config/generated/sdkconfig.defaults`
  - `config/generated/basalt.features.json`
- Add/update CLI tests (happy path + key failure cases).

Definition of done:
- CLI wizard passes smoke tests across representative boards (ESP32, STM32, RP2040, AVR).
- No silent failures; all failures return actionable messages.

### Sprint 3 (Weeks 5-6): Local configurator simplification (`v0.1.0` gate 2)

Goals:
- Keep web configurator as a dev utility, not a platform prototype.
- Remove/avoid scope that belongs to future `basaltos.io`.

Deliverables:
- `tools/basaltos_config_server.py` and `tools/basaltos_config_gui.html` tuned for:
  - board selection
  - driver selection
  - pin conflict detection
  - config preview and generation
- UX cleanup for clarity and speed.
- Update `tools/CONFIGURATOR_README.md` with exact supported scope.

Definition of done:
- Local configurator can generate valid outputs for target sample boards.
- No account/profile/marketplace-like flows in local tool.

### Sprint 4 (Weeks 7-8): Metadata quality and board expansion (`v0.1.0` gate 3)

Goals:
- Raise confidence in board/module metadata as source of truth.
- Improve new-board onboarding.

Deliverables:
- Strengthened validation paths (`tools/validate_metadata.py` and related checks).
- Fill high-value board metadata gaps (prioritize commonly used boards).
- Improve `docs/boards.md` and board schema examples.

Definition of done:
- Validation passes for all repo board/module metadata.
- New board addition workflow documented and reproducible in under 30 minutes.

### Sprint 5 (Weeks 9-10): Release readiness and `v0.1.0`

Goals:
- Ship `v0.1.0` with clear release quality signals.

Deliverables:
- Release checklist and sign-off process documented.
- Changelog updated with categorized changes and migration notes.
- End-to-end smoke workflow documented:
  - configure
  - build
  - flash
  - basic runtime verification

Definition of done:
- `v0.1.0` tag can be created with all quality gates green.
- Quickstart docs allow a new contributor to first success without manual intervention.

### Sprint 6 (Weeks 11-12): PlatformIO phase 1 kickoff (`v0.2.0` foundation)

Goals:
- Start PlatformIO support as the next major developer entry point.

Deliverables:
- Initial PlatformIO platform skeleton in separate integration workspace/repo direction.
- Scope boundary doc for v0.2.0 tracks: `docs/V0_2_SCOPE_BOUNDARIES.md`.
- Draft `platform.json` and builder script approach validated against BasaltOS generation flow.
- One reference board building through PlatformIO path.
- Publish implementation plan for full PlatformIO phase.

Definition of done:
- At least one board builds through initial PlatformIO flow using BasaltOS framework config.
- Risks and blockers captured with owners and dates.

## Backlog for next horizon (after week 12)

1. PlatformIO phase 1 completion and registry publish.
2. Basic VS Code extension (board selection + config generation).
3. App marketplace integration in extension (API-backed, future phase).
4. Advanced IDE features (pin mapper, analyzer, OTA).
5. Separate `basaltos.io` platform repository and service architecture.

## Explicitly out of scope for this roadmap window

- User accounts, social features, payment, cloud profiles.
- Hosted build/telemetry systems in this repository.
- Turning local configurator into a SaaS-style product.

## Tracking model

Use this status legend in PRs/issues:

- `planned`
- `in-progress`
- `blocked`
- `done`

Suggested issue labels:

- `roadmap:s1` ... `roadmap:s6`
- `area:cli`
- `area:configurator`
- `area:metadata`
- `area:docs`
- `area:platformio`
- `release:v0.1.0`

## Weekly review checklist

1. Did we ship sprint commitments?
2. Any scope drift toward future platform features?
3. Are generated config outputs stable?
4. Are docs still accurate for a new contributor?
5. What is blocked, by whom, and by what date will it be unblocked?
