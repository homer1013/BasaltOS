# Basalt Standard Library (v0.1 Draft)

## basalt.fs (planned)
- read(path)
- write(path, data)
- list(path)
- mkdir(path)

## basalt.gpio (current)
- mode(pin, mode)  # mode: 0=input, 1=output
- set(pin, value)
- get(pin)

## basalt.timer (current)
- sleep_ms(ms)

## basalt.led (current)
- set(r, g, b)
- off()

## basalt.events
- subscribe(topic, callback)
- publish(topic, data)

## basalt.log
- info(msg)
- warn(msg)
- error(msg)

## basalt.net (v0.1 minimal)
- wifi_connect(ssid, passphrase)
- http_get(url)

## basalt.ui (future)
- label(text)
- button(text, on_click)
- layout_vstack(items)

## basalt.audio (future)
- play_wav(path)
