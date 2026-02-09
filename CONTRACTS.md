# BasaltOS Main Contracts

`BasaltOS_Main` is the source-of-truth for core runtime, CLI/configure behavior, and metadata schemas.

## Ownership
- Core runtime/kernel, shell, drivers, metadata
- CLI and local configurator generation contracts
- Validation and CI gates for schema/config outputs

## Downstream Consumers
- `BasaltOS_Platform`
- `BasaltOS_PlatformIO`

Both consumers must pin to tagged releases from this repo.

## Stable Contract Surface
- `config/generated/basalt.features.json`
- `config/generated/sdkconfig.defaults`
- `config/generated/basalt_config.h`
- CLI invocation semantics for `tools/configure.py`

## Versioning Rules
- Non-breaking contract changes: allowed in minor release line.
- Breaking contract/schema changes: require changelog + migration notes.
- Consumers should update on tags, not arbitrary `main` SHAs.
