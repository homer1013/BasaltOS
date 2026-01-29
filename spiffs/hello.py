print("Hello from Basalt MicroPython!")
try:
    import basalt
    basalt.gpio.mode(2, 1)
    basalt.gpio.set(2, 1)
    basalt.timer.sleep_ms(200)
    basalt.gpio.set(2, 0)
except Exception as e:
    print("basalt module error:", e)
