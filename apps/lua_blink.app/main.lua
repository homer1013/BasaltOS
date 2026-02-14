-- Foundation sample for Lua gpio/timer binding parity checks.
local PIN = 2

basalt.system.log("lua_blink: init")
basalt.gpio.mode(PIN, 1)

for i = 1, 4 do
  basalt.gpio.write(PIN, i % 2)
  basalt.timer.sleep_ms(150)
end

basalt.gpio.write(PIN, 0)
basalt.system.log("lua_blink: done")
