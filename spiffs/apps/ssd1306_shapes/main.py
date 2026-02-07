print("[ssd1306_shapes] start")

try:
    import basalt
except Exception as e:
    basalt = None
    print("ssd1306_shapes: basalt import failed:", e)

if not basalt or not hasattr(basalt, "ssd1306"):
    print("ssd1306_shapes: basalt.ssd1306 unavailable (enable display_ssd1306 module)")
else:
    d = basalt.ssd1306

    try:
        if not d.ready():
            print("ssd1306_shapes: display not detected on I2C")
        else:
            w = d.width()
            h = d.height()
            cx = w // 2
            cy = h // 2
            wh_min = w if w < h else h
            rx = (w // 3) if (w // 3) > 8 else 8
            ry = (h // 4) if (h // 4) > 6 else 6

            d.clear()
            d.rect(0, 0, w, h, 1, False)
            d.rect(4, 4, w - 8, h - 8, 1, False)
            d.circle(cx, cy, wh_min // 4, 1, False)
            d.ellipse(cx, cy, rx, ry, 1, False)
            d.line(0, 0, w - 1, h - 1, 1)
            d.line(w - 1, 0, 0, h - 1, 1)
            if hasattr(d, "text_at"):
                d.text_at(2, 2, "DBG SSD1306", 1)
            d.show()
            print("ssd1306_shapes: rendered")
    except Exception as e:
        print("ssd1306_shapes error:", e)

print("[ssd1306_shapes] done")
