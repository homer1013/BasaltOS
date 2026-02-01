// C UI demo stub for BasaltOS native apps.
// This is a template for a future native app runtime.

#include <stdbool.h>
#include <stdint.h>

// Placeholder app state
static bool g_led_on = false;

static void app_init(void) {
    // TODO: init UI widgets + GPIO via Basalt HAL
    g_led_on = false;
}

static void app_tick(void) {
    // TODO: UI event handling + draw updates
    // Example: toggle a GPIO/LED at some interval
    g_led_on = !g_led_on;
}

int main(void) {
    app_init();

    // Simple app loop
    for (;;) {
        app_tick();
        // TODO: add delay hook / scheduler integration
    }

    return 0;
}
