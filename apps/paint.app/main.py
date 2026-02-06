print("[paint] start")
print("Basalt Paint (prototype)")
print("controls: touch/drag support depends on runtime UI/touch modules")

try:
    import basalt
except Exception as e:
    basalt = None
    print("paint: basalt module unavailable:", e)

if basalt and hasattr(basalt, "ui") and hasattr(basalt.ui, "text"):
    try:
        if hasattr(basalt.ui, "ready") and basalt.ui.ready():
            if hasattr(basalt.ui, "clear"):
                basalt.ui.clear()
            if hasattr(basalt.ui, "color"):
                basalt.ui.color(0xFFE0)  # yellow
            if hasattr(basalt.ui, "text_at"):
                basalt.ui.text_at(0, 0, "Basalt Paint")
                basalt.ui.text_at(0, 10, "Prototype UI API")
            basalt.ui.text("touch input wiring next")
            if hasattr(basalt.ui, "rect"):
                basalt.ui.rect(10, 30, 90, 50, 0xF800, False)
                basalt.ui.rect(120, 30, 90, 50, 0x07E0, True)
            if hasattr(basalt.ui, "circle"):
                basalt.ui.circle(55, 115, 24, 0x001F, False)
                basalt.ui.circle(165, 115, 24, 0xF81F, True)
            if hasattr(basalt.ui, "ellipse"):
                basalt.ui.ellipse(110, 180, 60, 24, 0xFFE0, False)
            if hasattr(basalt.ui, "line"):
                basalt.ui.line(8, 220, 220, 220, 0xFFFF)
                basalt.ui.line(8, 230, 220, 260, 0x07FF)
            if hasattr(basalt.ui, "text_at"):
                basalt.ui.text_at(0, 280, "Touch+drag to paint")
                basalt.ui.text_at(0, 290, "Top-left corner clears")

            # Interactive brush loop (stop from shell with `stop`/`kill`).
            if hasattr(basalt.ui, "touch") and hasattr(basalt.ui, "circle"):
                last_clear = 0
                while True:
                    t = basalt.ui.touch()
                    pressed = int(t[0]) if t else 0
                    if pressed:
                        tx = int(t[1])
                        ty = int(t[2])
                        # Clear gesture: tap top-left.
                        if tx < 30 and ty < 30:
                            now = 0
                            try:
                                import time
                                now = time.ticks_ms()
                            except Exception:
                                pass
                            if now == 0 or abs(now - last_clear) > 400:
                                basalt.ui.clear()
                                if hasattr(basalt.ui, "text_at"):
                                    basalt.ui.text_at(0, 0, "Basalt Paint")
                                    basalt.ui.text_at(0, 10, "cleared")
                                last_clear = now
                        else:
                            basalt.ui.circle(tx, ty, 4, 0xFFFF, True)
                    if hasattr(basalt, "timer"):
                        basalt.timer.sleep_ms(12)
                    else:
                        break
        else:
            basalt.ui.text("Paint prototype active")
        print("paint: drew status text via basalt.ui")
    except Exception as e:
        print("paint: ui draw failed:", e)
else:
    print("paint: ui API not available in this firmware build")

print("paint: TODO interactive brush canvas (touch + frame buffer)")
print("[paint] done")
