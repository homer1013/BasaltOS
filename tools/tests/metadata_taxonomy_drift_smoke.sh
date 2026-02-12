#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

python3 - <<'PY'
import json
from pathlib import Path

boards = sorted(Path("boards").rglob("board.json"))
if not boards:
    raise SystemExit("FAIL: no board.json files found")

platform_manufacturer = {
    "esp32": "Espressif",
    "avr": "Microchip",
    "atmega": "Microchip",
    "pic16": "Microchip",
    "rp2040": "Raspberry Pi",
    "stm32": "STMicroelectronics",
    "atsam": "Microchip",
    "renesas_ra": "Renesas",
    "renesas_rx": "Renesas",
    "linux-sbc": "Linux SBC",
}

platform_arch = {
    "avr": "AVR 8-bit",
    "atmega": "AVR 8-bit",
    "pic16": "PIC 8-bit",
    "stm32": "ARM Cortex-M",
    "atsam": "ARM Cortex-M",
    "rp2040": "ARM Cortex-M0+",
    "renesas_ra": "ARM Cortex-M",
    "renesas_rx": "Renesas RX 32-bit",
    "linux-sbc": "Linux Host",
}

def mcu_family(mcu: str, platform: str) -> str:
    txt = (mcu or "").upper()
    p = (platform or "").lower()
    if p == "esp32":
        if "C6" in txt: return "ESP32-C6"
        if "C3" in txt: return "ESP32-C3"
        if "S3" in txt: return "ESP32-S3"
        if "S2" in txt: return "ESP32-S2"
        return "ESP32"
    if p == "rp2040":
        return "RP2040"
    if p == "stm32":
        return "STM32"
    if p in ("avr", "atmega"):
        return "ATmega"
    if p == "pic16":
        return "PIC16"
    return p.upper() if p else "General"

errors = []
manufacturers = set()
architectures = set()
families = set()
silicons = set()

for bf in boards:
    try:
        d = json.loads(bf.read_text(encoding="utf-8"))
    except Exception as e:
        errors.append(f"{bf}: invalid JSON ({e})")
        continue

    platform = str(d.get("platform") or "")
    mcu = str(d.get("mcu") or d.get("soc") or d.get("target_profile") or "")
    manu = str(d.get("manufacturer") or platform_manufacturer.get(platform, "Generic")).strip()
    arch = str(d.get("architecture") or platform_arch.get(platform, "Embedded")).strip()
    fam = str(d.get("family") or mcu_family(mcu, platform)).strip()
    silicon = str(mcu or d.get("target_profile") or d.get("id") or "Unknown").strip()

    if not manu: errors.append(f"{bf}: empty manufacturer")
    if not arch: errors.append(f"{bf}: empty architecture")
    if not fam: errors.append(f"{bf}: empty family")
    if not silicon: errors.append(f"{bf}: empty silicon")

    manufacturers.add(manu)
    architectures.add(arch)
    families.add(fam)
    silicons.add(silicon)

if errors:
    print("FAIL: taxonomy derivation errors:")
    for e in errors:
        print(" -", e)
    raise SystemExit(1)

if len(manufacturers) < 3:
    raise SystemExit(f"FAIL: expected >=3 manufacturers, got {len(manufacturers)}")
if len(architectures) < 3:
    raise SystemExit(f"FAIL: expected >=3 architectures, got {len(architectures)}")
if len(families) < 5:
    raise SystemExit(f"FAIL: expected >=5 families, got {len(families)}")

print("PASS: metadata taxonomy drift smoke")
print(f"manufacturers={len(manufacturers)} architectures={len(architectures)} families={len(families)} silicons={len(silicons)}")
PY
