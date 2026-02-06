print("[tft_hello] start")
msg = "Hello from BasaltOS applet"
print(msg)
try:
    import basalt
    if hasattr(basalt, "ui") and hasattr(basalt.ui, "text"):
        basalt.ui.text(msg)
        print("tft_hello: rendered with basalt.ui.text")
except Exception as e:
    print("tft_hello: ui path unavailable:", e)
print("[tft_hello] done")
