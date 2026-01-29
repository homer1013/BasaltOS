import basalt
import basalt.ui as ui


def main():
    print("UI demo (stub) running")

    screen = ui.screen()
    ui.set_title("UI Demo")

    label = ui.label("LED: OFF", x=10, y=10)
    button = ui.button("Toggle LED", x=10, y=40, w=120, h=30)

    state = {"on": False}

    def on_press():
        state["on"] = not state["on"]
        label.set_text("LED: ON" if state["on"] else "LED: OFF")
        if state["on"]:
            basalt.led.set(1, 0, 0)
        else:
            basalt.led.off()

    button.on_press(on_press)
    screen.add(label)
    screen.add(button)
    screen.show()

    # Keep running so callbacks can fire in the future UI implementation.
    while True:
        basalt.timer.sleep_ms(250)


if __name__ == "__main__":
    main()
