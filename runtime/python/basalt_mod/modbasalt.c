#include "py/obj.h"
#include "py/runtime.h"

#include "board_config.h"
#include "tft_console.h"

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
    if (BASALT_LED_R_PIN >= 0) io_conf.pin_bit_mask |= (1ULL << BASALT_LED_R_PIN);
    if (BASALT_LED_G_PIN >= 0) io_conf.pin_bit_mask |= (1ULL << BASALT_LED_G_PIN);
    if (BASALT_LED_B_PIN >= 0) io_conf.pin_bit_mask |= (1ULL << BASALT_LED_B_PIN);
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

// basalt.ui.ready() -> bool
static mp_obj_t basalt_ui_ready(void) {
    return mp_obj_new_bool(tft_console_is_ready());
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_ready_obj, basalt_ui_ready);

// basalt.ui.clear()
static mp_obj_t basalt_ui_clear(void) {
    if (tft_console_is_ready()) {
        tft_console_clear();
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_clear_obj, basalt_ui_clear);

// basalt.ui.text(msg)
static mp_obj_t basalt_ui_text(mp_obj_t msg_obj) {
    const char *msg = mp_obj_str_get_str(msg_obj);
    if (tft_console_is_ready() && msg) {
        tft_console_write(msg);
        tft_console_write("\n");
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_text_obj, basalt_ui_text);

// basalt.ui.text_at(x, y, msg)
static mp_obj_t basalt_ui_text_at(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t msg_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    const char *msg = mp_obj_str_get_str(msg_obj);
    if (tft_console_is_ready() && msg) {
        tft_console_write_at(x, y, msg);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(basalt_ui_text_at_obj, basalt_ui_text_at);

// basalt.ui.color(rgb565_int)
static mp_obj_t basalt_ui_color(mp_obj_t color_obj) {
    int c = mp_obj_get_int(color_obj);
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    if (tft_console_is_ready()) {
        tft_console_set_color((uint16_t)c);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_color_obj, basalt_ui_color);

// basalt.ui.pixel(x, y, color)
static mp_obj_t basalt_ui_pixel(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t color_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int c = mp_obj_get_int(color_obj);
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_pixel(x, y, (uint16_t)c);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(basalt_ui_pixel_obj, basalt_ui_pixel);

// basalt.ui.line(x0, y0, x1, y1, color)
static mp_obj_t basalt_ui_line(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int x0 = mp_obj_get_int(args[0]);
    int y0 = mp_obj_get_int(args[1]);
    int x1 = mp_obj_get_int(args[2]);
    int y1 = mp_obj_get_int(args[3]);
    int c = mp_obj_get_int(args[4]);
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_line(x0, y0, x1, y1, (uint16_t)c);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_line_obj, 5, 5, basalt_ui_line);

// basalt.ui.rect(x, y, w, h, color[, fill])
static mp_obj_t basalt_ui_rect(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    int c = mp_obj_get_int(args[4]);
    bool fill = (n_args >= 6) ? mp_obj_is_true(args[5]) : false;
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_rect(x, y, w, h, (uint16_t)c, fill);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_rect_obj, 5, 6, basalt_ui_rect);

// basalt.ui.circle(cx, cy, r, color[, fill])
static mp_obj_t basalt_ui_circle(size_t n_args, const mp_obj_t *args) {
    int cx = mp_obj_get_int(args[0]);
    int cy = mp_obj_get_int(args[1]);
    int r = mp_obj_get_int(args[2]);
    int c = mp_obj_get_int(args[3]);
    bool fill = (n_args >= 5) ? mp_obj_is_true(args[4]) : false;
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_circle(cx, cy, r, (uint16_t)c, fill);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_circle_obj, 4, 5, basalt_ui_circle);

// basalt.ui.ellipse(cx, cy, rx, ry, color[, fill])
static mp_obj_t basalt_ui_ellipse(size_t n_args, const mp_obj_t *args) {
    int cx = mp_obj_get_int(args[0]);
    int cy = mp_obj_get_int(args[1]);
    int rx = mp_obj_get_int(args[2]);
    int ry = mp_obj_get_int(args[3]);
    int c = mp_obj_get_int(args[4]);
    bool fill = (n_args >= 6) ? mp_obj_is_true(args[5]) : false;
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_ellipse(cx, cy, rx, ry, (uint16_t)c, fill);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_ellipse_obj, 5, 6, basalt_ui_ellipse);

// basalt.ui.touch() -> (pressed, x, y, raw_x, raw_y)
static mp_obj_t basalt_ui_touch(void) {
    int pressed = 0;
    int x = -1;
    int y = -1;
    int raw_x = -1;
    int raw_y = -1;
    tft_console_touch_read(&pressed, &x, &y, &raw_x, &raw_y);

    mp_obj_t items[5];
    items[0] = mp_obj_new_int(pressed);
    items[1] = mp_obj_new_int(x);
    items[2] = mp_obj_new_int(y);
    items[3] = mp_obj_new_int(raw_x);
    items[4] = mp_obj_new_int(raw_y);
    return mp_obj_new_tuple(5, items);
}
static MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_touch_obj, basalt_ui_touch);

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
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ready")),
        MP_OBJ_FROM_PTR(&basalt_ui_ready_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("clear")),
        MP_OBJ_FROM_PTR(&basalt_ui_clear_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("text")),
        MP_OBJ_FROM_PTR(&basalt_ui_text_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("text_at")),
        MP_OBJ_FROM_PTR(&basalt_ui_text_at_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("color")),
        MP_OBJ_FROM_PTR(&basalt_ui_color_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("pixel")),
        MP_OBJ_FROM_PTR(&basalt_ui_pixel_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("line")),
        MP_OBJ_FROM_PTR(&basalt_ui_line_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("rect")),
        MP_OBJ_FROM_PTR(&basalt_ui_rect_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("circle")),
        MP_OBJ_FROM_PTR(&basalt_ui_circle_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("ellipse")),
        MP_OBJ_FROM_PTR(&basalt_ui_ellipse_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("touch")),
        MP_OBJ_FROM_PTR(&basalt_ui_touch_obj));

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
