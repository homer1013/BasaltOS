# Sprint 2 CLI Reliability Acceptance Matrix

This matrix is the primary acceptance artifact for `SCRUM-62` and the execution baseline for `SCRUM-63`.

## Scope

Target command surface:

- `tools/configure.py`
- `tools/board.sh`

Goal: enforce deterministic, actionable, and non-silent CLI behavior across supported board metadata.

## Run Setup

```bash
DATE=$(date +%F)
OUT="tmp/acceptance/s2-cli/$DATE"
mkdir -p "$OUT/artifacts"
exec > >(tee -a "$OUT/commands.log") 2>&1
```

Shortcut runner:

```bash
bash tools/tests/run_s2_cli_matrix.sh
```

## Matrix

| ID | Area | Command / Flow | Expected Result | Evidence | Gate |
|---|---|---|---|---|---|
| CLI-01 | Discovery | `python3 tools/configure.py --list-boards` | Exit `0`; boards listed from multiple platforms | `commands.log` | Must Pass |
| CLI-02 | Discovery | `python3 tools/configure.py --list-drivers` | Exit `0`; non-empty driver list | `commands.log` | Must Pass |
| CLI-03 | Help/UX | `python3 tools/configure.py --help` | Exit `0`; includes platform/board usage text | `commands.log` | Must Pass |
| CLI-04 | Generation (ESP32) | `python3 tools/configure.py --platform esp32 --board esp32-c3-supermini --enable-drivers uart --outdir "$OUT/artifacts/esp32"` | Exit `0`; files generated | `artifacts/esp32/*` | Must Pass |
| CLI-05 | Generation (STM32) | `python3 tools/configure.py --platform stm32 --board nucleo_f401re --enable-drivers uart --outdir "$OUT/artifacts/stm32"` | Exit `0`; files generated | `artifacts/stm32/*` | Must Pass |
| CLI-06 | Generation (RP2040) | `python3 tools/configure.py --platform rp2040 --board raspberry_pi_pico --enable-drivers uart --outdir "$OUT/artifacts/rp2040"` | Exit `0`; files generated | `artifacts/rp2040/*` | Must Pass |
| CLI-07 | Generation (AVR) | `python3 tools/configure.py --platform atmega --board arduino_uno_r3 --enable-drivers uart --outdir "$OUT/artifacts/avr"` | Exit `0`; files generated | `artifacts/avr/*` | Must Pass |
| CLI-08 | Invalid board hard-fail | `python3 tools/configure.py --platform esp32 --board does-not-exist --enable-drivers uart` | Exit non-zero; message contains invalid board context | `commands.log` | Must Pass |
| CLI-08b | Negative-path bundle | `bash tools/tests/configure_negative_paths.sh` | Exit `0`; all failure-path assertions pass | `commands.log` | Must Pass |
| CLI-09 | Selector script syntax | `bash -n tools/board.sh` | Exit `0` | `commands.log` | Must Pass |
| CLI-10 | Selector list | `bash tools/board.sh --list` | Exit `0`; list output present | `commands.log` | Must Pass |
| CLI-11 | Determinism | `bash tools/tests/configure_deterministic_outputs.sh` | Exit `0`; normalized outputs match across runs | `commands.log` | Must Pass |
| CLI-12 | Broad smoke | `bash tools/tests/configure_smoke_multi_board.sh` | Exit `0`; full board sweep passes | `commands.log` | Must Pass |

## Artifact Minimum

For each generated board output folder, all files must exist:

- `basalt_config.h`
- `basalt.features.json`
- `basalt_config.json`
- `sdkconfig.defaults`

## Failure Policy

A matrix row is `FAIL` when any expected result is unmet.

On `FAIL`:

1. Create Jira bug/task linked to `SCRUM-63`.
2. Include failing command and stderr excerpt.
3. Mark row as `FAIL` with issue key.

## Completion Criteria (SCRUM-62)

`SCRUM-62` is complete when:

1. This matrix is committed and linked in Jira.
2. All matrix commands are runnable from documented setup.
3. At least one execution pass is logged with evidence paths.
