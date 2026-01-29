# Basalt OS v0.1 Spec (Draft)

## Vision
Basalt OS is a lightweight embedded OS that makes building hardware projects feel like building apps. It provides a friendly CLI, a filesystem, and installable apps with a stable API surface.

## Target hardware
- Primary: ESP32-S3 + PSRAM
- Secondary: ESP32 (classic)

## Architecture
```
+------------------------------+
| CLI / UX Shell (bsh)         |
+------------------------------+
| App Manager / Permissions    |
+------------------------------+
| Basalt Standard Library APIs |
+------------------------------+
| Kernel Services              |
+------------------------------+
| HAL + Drivers                |
+------------------------------+
| ESP-IDF / FreeRTOS (initial) |
+------------------------------+
```

## App model
Apps are folders with a manifest and entrypoint.

Example layout:
```
example.app/
├── app.toml
├── main.py
└── assets/
```

Example manifest:
```toml
name = "Blink Demo"
version = "0.1.0"
entry = "main.py"
runtime = "python"

[capabilities]
gpio = ["read", "write"]
timer = true
ui = false
audio = false
net = false
```

## CLI (bsh)
Core commands:
- help, ls, cd, cat, edit
- run, apps, install, remove
- top, logs, wifi, reboot

Wizard commands:
- new sensor-project
- add button
- add display
- add wifi

## Basalt Standard Library (v0.1)
- basalt.fs
- basalt.gpio
- basalt.i2c
- basalt.spi
- basalt.uart
- basalt.timer
- basalt.events
- basalt.log
- basalt.net
- basalt.audio
- basalt.ui

## Milestones (v0.1)
1) Boot + serial console
2) LittleFS mount
3) bsh shell
4) App loader
5) One runtime (python or lua)
6) GPIO API
7) Hello/Blink/Sensor demo apps

