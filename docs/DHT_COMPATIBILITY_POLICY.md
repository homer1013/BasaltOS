# DHT Compatibility Policy (SCRUM-98)

## Scope

Basalt runtime command `dht22` supports DHT22 and DHT11 framing compatibility
using a single command surface.

## Command Contract

```text
dht22 status
dht22 read [pin] [auto|dht22|dht11]
```

- `pin` defaults to `BASALT_PIN_DHT22_DATA`.
- `mode` defaults to `BASALT_CFG_DHT_SENSOR_TYPE` (`"auto"` if unset).

## Detection / Decode Rules

1. Read 40-bit DHT frame (shared transport for DHT11/DHT22).
2. Verify checksum.
3. Decode both interpretations:
   - DHT22: 16-bit humidity + 16-bit temperature (signed, tenths)
   - DHT11: integer bytes + decimal bytes
4. Apply mode:
   - `dht22`: force DHT22 decode
   - `dht11`: force DHT11 decode
   - `auto`: prefer DHT22 when plausible; otherwise use DHT11

### Plausibility guards

- DHT22 plausible: humidity `0..100`, temperature `-40..125`.
- DHT11 plausible: humidity `0..100`, temperature `0..80`, decimal bytes `<=9`.

## Runtime Outputs

`dht22 read ...` prints:

- `mode_used`
- `sensor_detected`
- `decode_valid.dht22`
- `decode_valid.dht11`
- normalized `temp_c` and `humidity_pct`
- raw bytes for audit/debug

## Bench Note

On the validated DHT22 module (GPIO10), `auto` selects `dht22` and reports
expected humidity/temperature values.

With a DHT11 module connected on GPIO10:

- `dht22 read 10 auto` selected `dht11`
- `dht22 read 10 dht11` produced valid sample output
- `dht22 read 10 dht22` failed/invalid as expected for DHT11
