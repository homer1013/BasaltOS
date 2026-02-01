from basalt import ui, timer, led

# Minimal UI + log demo using the basalt.ui stub API.
# Keep calls positional to avoid keyword parsing issues.

screen = ui.screen()
label = ui.label("LED OFF", 10, 10)
screen.add(label)
screen.show()

state_on = False
while True:
    state_on = not state_on
    if state_on:
        led.set(0, 1, 0)
        label.set_text("LED ON")
        print("py_ui_demo: LED ON")
    else:
        led.off()
        label.set_text("LED OFF")
        print("py_ui_demo: LED OFF")
    # Keep each state long enough to see with the naked eye.
    timer.sleep_ms(1500)
