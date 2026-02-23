# HAL Stabilization Bench Validation (2026-02-23)

## Scope
- Stabilization of shared bus ownership path (`main/bus_manager.c`) and HAL target wiring (`basalt_hal/CMakeLists.txt`).
- Safety hardening of ESP32-family SPI reconfiguration path (`hal_spi_set_freq` / `hal_spi_set_mode`).
- Safety hardening of ESP32-family SPI transfer length bounds (`size_t` -> IDF `int` bit-length field).
- Safety hardening of ESP32-family I2C shared-driver ownership and timeout conversion semantics.
- Safety hardening of ESP32-family I2C/UART I/O length bounds (`size_t` -> `int` return semantics).
- Safety hardening of ESP32-family UART shared-driver ownership.
- Contract lock for ESP32-family GPIO IRQ service behavior (shared ISR service semantics).
- Validation on local CI-style smoke suite and physical ESP32 hardware.

## Build Validation
- ESP-IDF env: `/home/homer/Projects/esp/esp-idf` (`v6.1` toolchain)
- Target builds:
  - `build_hal_stab_esp32`: PASS
  - `build_hal_stab_esp32_clean` (isolated sdkconfig): PASS
  - `build_hal_stab_esp32c3` (isolated sdkconfig): PASS
  - `build_hal_stab_esp32c6` (isolated sdkconfig): PASS
  - `build_hal_stab_esp32s3`: PASS
  - Fresh isolated matrix after UART/I2C updates:
    - `build_hal_fresh_esp32`: PASS
    - `build_hal_fresh_esp32c3`: PASS
    - `build_hal_fresh_esp32c6`: PASS
    - `build_hal_fresh_esp32s3`: PASS
  - Bounds hardening compile sanity:
    - `build_hal_bounds_esp32`: PASS
    - `build_hal_bounds_esp32c3`: PASS

Build artifacts:
- `build_hal_stab_esp32/basaltos.bin`
- `build_hal_stab_esp32s3/basaltos.bin`
- `build_hal_bounds_esp32/basaltos.bin`
- `build_hal_bounds_esp32c3/basaltos.bin`

## Hardware Bench
- Device port: `/dev/ttyACM0`
- Chip probe:
  - Chip: `ESP32-PICO-V3-02`
  - MAC: `4c:c3:82:9b:7a:8c`

### Flash
- Flashed with:
  - `python -m esptool --chip esp32 -p /dev/ttyACM0 -b 460800 --before default-reset --after hard-reset write-flash @flash_args`
  - from `build_hal_stab_esp32/`
- Result: PASS

### Runtime Evidence
- Boot + POST:
  - `Basalt OS booted. Type 'help'.`
  - `[smoke] === BasaltOS POST PASS ===`
- Post SPI-safety-fix reflashed binary (`build_hal_stab_esp32_clean`) also shows:
  - `Basalt OS booted. Type 'help'.`
  - `[smoke] === BasaltOS POST PASS ===`
- Post UART/I2C hardening reflashed binary (`build_hal_fresh_esp32`) also shows:
  - `Basalt OS booted. Type 'help'.`
  - `[smoke] === BasaltOS POST PASS ===`
- CLI/HAL command proof:
  - Command: `uart status`
  - Response includes:
    - `uart.enabled: yes`
    - `uart.cfg.port: 0`
    - `uart.cfg.baudrate: 115200`
    - `uart.runtime.loopback_port: 1`
  - Command: `help`
  - Response includes:
    - `Basalt shell. Type 'help -commands' for the list, or 'help <cmd>' for details.`

## Test Gates
- `tools/tests/main_hardening_smoke_bundle.sh`: PASS (`47/47`)
- New guardrails:
  - `tools/tests/bus_manager_usage_smoke.sh`
  - `tools/tests/hal_cmake_target_contract_smoke.sh`
  - `tools/tests/hal_spi_reconfigure_safety_smoke.sh`
  - `tools/tests/hal_spi_transfer_length_bound_smoke.sh`
  - `tools/tests/hal_i2c_driver_ownership_smoke.sh`
  - `tools/tests/hal_i2c_timeout_semantics_smoke.sh`
  - `tools/tests/hal_uart_driver_ownership_smoke.sh`
  - `tools/tests/hal_uart_init_rollback_smoke.sh`
  - `tools/tests/hal_io_length_bounds_smoke.sh`
  - `tools/tests/hal_gpio_irq_service_contract_smoke.sh`

## Notes
- Flash-size warning (`8MB detected, 4MB image header`) is expected with current image settings and did not block boot or CLI.

## Post-Closure Bench Validation (2026-02-23 12:35 local)
- Target board: Arduino Uno R4 WiFi on `/dev/ttyACM0`
- Camera source used for live bench visibility: `/dev/video2` (`UVC Camera (046d:0825)`)

### Runtime Validation Commands
- Upload shell bench sketch:
  - `arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_shell_paint`
  - `arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:renesas_uno:unor4wifi tools/bench/uno_r4_tft_shell_paint`
- Serial probe commands sent:
  - `help`
  - `id`

### Evidence
- Serial capture file:
  - `/tmp/uno_r4_tft_shell_serial_2026-02-23.log`
- Camera capture file:
  - `/tmp/uno_r4_tft_shell_cam_2026-02-23.jpg`
- Observed serial responses:
  - `commands:` list returned for `help`
  - `lcd id: 0x6814` returned for `id`
- Observed display content in captured frame:
  - Header text: `Basalt UNO R4 bench`
  - Prompt text includes: `shell + paint + cal + flappy`
  - Shell status line includes: `Uno serial help`
