local PIN = 2
basalt.system.log("lua_blink spiffs: init")
basalt.gpio.mode(PIN, 1)
for i = 1, 4 do
  basalt.gpio.write(PIN, i % 2)
  basalt.timer.sleep_ms(120)
end
basalt.gpio.write(PIN, 0)
basalt.system.log("lua_blink spiffs: done")
