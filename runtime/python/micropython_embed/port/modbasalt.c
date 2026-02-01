#include "py/obj.h"
#include "py/runtime.h"

#include "board_config.h"
#include "modui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// basalt.gpio.mode(pin, mode) where mode: 0=input, 1=output
static mp_obj_t basalt_gpio_mode(mp_obj_t pin_obj, mp_obj_t mode_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int mode = mp_obj_get_int(mode_obj);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = (mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(basalt_gpio_mode_obj, basalt_gpio_mode);

// basalt.gpio.set(pin, value)
static mp_obj_t basalt_gpio_set(mp_obj_t pin_obj, mp_obj_t val_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int val = mp_obj_is_true(val_obj) ? 1 : 0;
    gpio_set_level(pin, val);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(basalt_gpio_set_obj, basalt_gpio_set);

// basalt.gpio.get(pin)
static mp_obj_t basalt_gpio_get(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int val = gpio_get_level(pin);
    return mp_obj_new_int(val);
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_gpio_get_obj, basalt_gpio_get);

// basalt.timer.sleep_ms(ms)
static mp_obj_t basalt_timer_sleep_ms(mp_obj_t ms_obj) {
    int ms = mp_obj_get_int(ms_obj);
    if (ms < 0) ms = 0;
    vTaskDelay(pdMS_TO_TICKS(ms));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_timer_sleep_ms_obj, basalt_timer_sleep_ms);

static void basalt_led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
#if BASALT_LED_R_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_R_PIN);
#endif
#if BASALT_LED_G_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_G_PIN);
#endif
#if BASALT_LED_B_PIN >= 0
    io_conf.pin_bit_mask |= (1ULL << BASALT_LED_B_PIN);
#endif
    if (io_conf.pin_bit_mask) {
        gpio_config(&io_conf);
    }
}

static void basalt_led_set_unique_pin(int pin, int r, int g, int b) {
    if (pin < 0) return;
    int want_on = 0;
    if (BASALT_LED_R_PIN >= 0 && BASALT_LED_R_PIN == pin) want_on |= r;
    if (BASALT_LED_G_PIN >= 0 && BASALT_LED_G_PIN == pin) want_on |= g;
    if (BASALT_LED_B_PIN >= 0 && BASALT_LED_B_PIN == pin) want_on |= b;
    int level = BASALT_LED_ACTIVE_LOW ? !want_on : want_on;
    gpio_set_level(pin, level);
}

// basalt.led.set(r, g, b)
static mp_obj_t basalt_led_set(mp_obj_t r_obj, mp_obj_t g_obj, mp_obj_t b_obj) {
    basalt_led_init();
    int r = mp_obj_is_true(r_obj) ? 1 : 0;
    int g = mp_obj_is_true(g_obj) ? 1 : 0;
    int b = mp_obj_is_true(b_obj) ? 1 : 0;

    // Handle boards where multiple channels map to the same physical pin.
    basalt_led_set_unique_pin(BASALT_LED_R_PIN, r, g, b);
    if (BASALT_LED_G_PIN != BASALT_LED_R_PIN) {
        basalt_led_set_unique_pin(BASALT_LED_G_PIN, r, g, b);
    }
    if (BASALT_LED_B_PIN != BASALT_LED_R_PIN && BASALT_LED_B_PIN != BASALT_LED_G_PIN) {
        basalt_led_set_unique_pin(BASALT_LED_B_PIN, r, g, b);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(basalt_led_set_obj, basalt_led_set);

// basalt.led.off()
static mp_obj_t basalt_led_off(void) {
    int off = BASALT_LED_ACTIVE_LOW ? 1 : 0;
    if (BASALT_LED_R_PIN >= 0) gpio_set_level(BASALT_LED_R_PIN, off);
    if (BASALT_LED_G_PIN >= 0) gpio_set_level(BASALT_LED_G_PIN, off);
    if (BASALT_LED_B_PIN >= 0) gpio_set_level(BASALT_LED_B_PIN, off);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_led_off_obj, basalt_led_off);

void basalt_module_init(void) {
    qstr q_basalt = qstr_from_str("basalt");
    mp_obj_module_t *mod = mp_obj_new_module(q_basalt);

    mp_obj_module_t *gpio_mod = mp_obj_new_module(qstr_from_str("basalt_gpio"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("mode")),
        MP_OBJ_FROM_PTR(&basalt_gpio_mode_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("set")),
        MP_OBJ_FROM_PTR(&basalt_gpio_set_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(gpio_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("get")),
        MP_OBJ_FROM_PTR(&basalt_gpio_get_obj));

    mp_obj_module_t *timer_mod = mp_obj_new_module(qstr_from_str("basalt_timer"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(timer_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("sleep_ms")),
        MP_OBJ_FROM_PTR(&basalt_timer_sleep_ms_obj));

    mp_obj_module_t *led_mod = mp_obj_new_module(qstr_from_str("basalt_led"));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(led_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("set")),
        MP_OBJ_FROM_PTR(&basalt_led_set_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(led_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("off")),
        MP_OBJ_FROM_PTR(&basalt_led_off_obj));

    mp_obj_module_t *ui_mod = mp_obj_new_module(qstr_from_str("basalt_ui"));
    basalt_ui_init(ui_mod);

    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("gpio")),
        MP_OBJ_FROM_PTR(gpio_mod));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("timer")),
        MP_OBJ_FROM_PTR(timer_mod));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("led")),
        MP_OBJ_FROM_PTR(led_mod));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ui")),
        MP_OBJ_FROM_PTR(ui_mod));

    mp_store_global(q_basalt, MP_OBJ_FROM_PTR(mod));
}
