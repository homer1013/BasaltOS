# Basalt OS example app (placeholder runtime contract)
# This assumes a future basalt Python runtime with gpio + timer APIs.

from basalt import gpio, timer

LED_PIN = 2

led = gpio.output(LED_PIN)

while True:
    led.toggle()
    timer.sleep_ms(500)
