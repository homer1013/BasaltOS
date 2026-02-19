print("[sd_probe] start")
os_mod = None
try:
    import uos as os_mod
except:
    try:
        import os as os_mod
    except:
        os_mod = None

if os_mod is None:
    print("sd_probe: os/uos module unavailable in this MicroPython build")
else:
    try:
        entries = os_mod.listdir("/sd")
        print("sd.mounted: yes")
        print("sd.entries:", len(entries))
        i = 0
        for name in entries:
            print(" -", name)
            i = i + 1
            if i >= 20:
                break
    except:
        print("sd.mounted: no")
        print("sd_probe: listdir('/sd') failed")
print("[sd_probe] done")
