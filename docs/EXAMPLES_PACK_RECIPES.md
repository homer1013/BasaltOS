# BasaltOS Examples Pack (Copy/Paste Recipes)

This page is the fast path for three practical demos after first boot:
- OLED demo (`display_ssd1306`)
- DC motor demo (`l298n`)
- Sensor demo (`dht22`, DHT22/DHT11-compatible runtime command)

Reference board for commands below:
- `esp32-c3-supermini`

## 0) One-time generate/build/flash pattern

For each recipe, use:
1. `python3 tools/configure.py ...`
2. `SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build set-target esp32c3`
3. `SDKCONFIG_DEFAULTS=config/generated/sdkconfig.defaults idf.py -B build build`
4. `idf.py -B build -p /dev/ttyUSB0 flash monitor`

If your serial port is different, replace `/dev/ttyUSB0`.

---

## 1) OLED Demo Recipe (SSD1306)

Configure:
```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers gpio,uart,i2c,shell_full,display_ssd1306 \
  --outdir config/generated
```

In shell:
```text
basalt> drivers
basalt> help -commands
```

Expected signs:
- `drivers` includes `display_ssd1306: enabled`
- boot logs show SSD1306 init success when panel/wiring are correct

If needed, use the deeper hardware checklist:
- `docs/SSD1306_HARDWARE_VALIDATION.md`

---

## 2) DC Motor Demo Recipe (L298N)

Configure:
```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers gpio,uart,shell_full,l298n \
  --outdir config/generated
```

In shell:
```text
basalt> l298n status
basalt> l298n test
basalt> l298n a fwd
basalt> l298n a stop
basalt> l298n b fwd
basalt> l298n b stop
```

Expected signs:
- `l298n.enabled: yes`
- `l298n test: ...` then `l298n test: done`
- channel commands respond with channel state changes

If needed, use the deeper hardware checklist:
- `docs/L298N_ULN2003_HARDWARE_VALIDATION.md`

---

## 3) Sensor Demo Recipe (DHT22 / DHT11 module)

Configure:
```bash
python3 tools/configure.py \
  --platform esp32 \
  --board esp32-c3-supermini \
  --enable-drivers gpio,uart,shell_full,dht22 \
  --outdir config/generated
```

In shell (example pin `GPIO10`; change for your wiring):
```text
basalt> dht22 status
basalt> dht22 read 10 auto
```

Expected signs:
- `dht22 read: ok`
- `dht22.pin_used: <pin>`
- `dht22.temp_c: <value>`
- `dht22.humidity_pct: <value>`

If auto-detect misclassifies the module, force decode:
```text
basalt> dht22 read 10 dht22
basalt> dht22 read 10 dht11
```

If needed, use the deeper hardware checklist:
- `docs/DHT22_HARDWARE_VALIDATION.md`
- `docs/DHT_COMPATIBILITY_POLICY.md`

---

## Minimal “it works” checklist

You are done with the examples pack when all are true:
- OLED build boots and reports display driver enabled
- L298N commands run and `l298n test` completes
- DHT command returns valid temperature + humidity output
