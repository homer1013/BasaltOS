print("[system_info] start")
gc_mod = None
for mod_name in ("gc",):
    try:
        gc_mod = __import__(mod_name)
        break
    except Exception:
        pass
if gc_mod and hasattr(gc_mod, "mem_free"):
    try:
        print("heap.free:", gc_mod.mem_free())
    except Exception as e:
        print("heap.free: unavailable:", e)
else:
    print("heap.free: unavailable (gc module not present)")
try:
    import basalt
    print("basalt.led:", "yes" if hasattr(basalt, "led") else "no")
    print("basalt.timer:", "yes" if hasattr(basalt, "timer") else "no")
except Exception as e:
    print("basalt import error:", e)
print("[system_info] done")
