# Hero Board Quickstarts

Use these curated profiles to get to first success quickly:

- `esp32_c6_quickstart` -> `esp32/esp32-c6`
- `cyd_quickstart` -> `esp32/cyd_3248s035r`
- `mega2560_quickstart` -> `avr/mega2560`

Command pattern:

```bash
python tools/configure.py --quickstart <profile_id>
```

Then build from repo root:

```bash
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build set-target <target>
SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build
```
