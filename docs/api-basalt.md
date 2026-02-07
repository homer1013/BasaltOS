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

## basalt.rtc (current, basic)
- available() -> bool
- now() -> (year, month, day, hour, minute, second, weekday)

## basalt.ssd1306 (current, basic)
- ready() -> bool
- width() / height()
- clear([color])
- color(value)
- pixel(x, y[, color])
- line(x0, y0, x1, y1[, color])
- rect(x, y, w, h[, color[, fill]])
- circle(cx, cy, r[, color[, fill]])
- ellipse(cx, cy, rx, ry[, color[, fill]])
- text_at(x, y, text[, color])  # debug font helper
- text(text[, color])            # debug cursor helper
- show()

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

## basalt.ui (current text-mode implementation)
`basalt.ui` currently renders text widgets through the TFT console bridge when TFT console is enabled.
This is not a full graphics/UI stack yet; callbacks and touch integration are still placeholder-level.

Functions:
- screen()
- button(text, x=0, y=0, w=0, h=0)
- label(text, x=0, y=0)
- set_title(text)

Screen methods:
- add(widget)
- show()
- clear()

Button methods:
- on_press(callback)
- set_text(text)

Label methods:
- set_text(text)

## basalt.audio (future)
- play_wav(path)
