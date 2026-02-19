# CLI Foundation Quickstart

This document defines the fastest, repeatable CLI entry points for first-success builds.

## Fast Paths

List curated quickstart profiles:

```bash
python tools/configure.py --list-profiles
```

Generate from a profile:

```bash
python tools/configure.py --quickstart esp32_c6_quickstart
```

Generate from a board directly:

```bash
python tools/configure.py --platform esp32 --quickstart esp32-c6
```

Run preflight checks + command hints:

```bash
python tools/configure.py --doctor --platform esp32 --board esp32-c6
```

## Profile Reuse

Save a resolved config profile:

```bash
python tools/configure.py --quickstart esp32_c6_quickstart --profile-save my_demo_profile
```

Load saved profile:

```bash
python tools/configure.py --profile-load my_demo_profile
```

Profiles can be stored under `config/profiles/*.json` or passed by JSON path.

## Runtime Readiness

Show driver-level runtime readiness status:

```bash
python tools/configure.py --list-runtime-status
```

Generated runs also print readiness for enabled drivers to reduce runtime surprises.
