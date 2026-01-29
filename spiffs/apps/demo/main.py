print("Demo app running on Basalt OS")
try:
    import basalt

    for _ in range(3):
        basalt.led.set(1, 0, 0)
        basalt.timer.sleep_ms(150)
        basalt.led.set(0, 1, 0)
        basalt.timer.sleep_ms(150)
        basalt.led.set(0, 0, 1)
        basalt.timer.sleep_ms(150)
        basalt.led.off()
        basalt.timer.sleep_ms(150)
except Exception as e:
    print("basalt module error:", e)
finally:
    try:
        import basalt
        basalt.led.off()
    except Exception:
        pass
