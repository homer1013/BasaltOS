#include "py/obj.h"
#include "py/runtime.h"
#include "py/qstr.h"
#include "py/misc.h"
#include <string.h>

#include "modui.h"
#include "tft_console.h"

#ifndef STATIC
#define STATIC static
#endif

typedef struct _basalt_ui_screen_t {
    mp_obj_base_t base;
} basalt_ui_screen_t;

typedef struct _basalt_ui_button_t {
    mp_obj_base_t base;
    mp_obj_t text;
    mp_int_t last_len;
    mp_int_t x;
    mp_int_t y;
    mp_int_t w;
    mp_int_t h;
} basalt_ui_button_t;

typedef struct _basalt_ui_label_t {
    mp_obj_base_t base;
    mp_obj_t text;
    mp_int_t last_len;
    mp_int_t x;
    mp_int_t y;
} basalt_ui_label_t;

// Forward declarations for type objects used before their definitions.
extern const mp_obj_type_t basalt_ui_label_type;
extern const mp_obj_type_t basalt_ui_button_type;

typedef enum {
    BASALT_UI_ITEM_LABEL = 1,
    BASALT_UI_ITEM_BUTTON = 2,
} basalt_ui_item_kind_t;

typedef struct {
    basalt_ui_item_kind_t kind;
    mp_obj_t obj;
} basalt_ui_item_t;

#define BASALT_UI_MAX_ITEMS 16
static basalt_ui_item_t s_items[BASALT_UI_MAX_ITEMS];
static int s_item_count = 0;

// ------------------------
// Screen methods (no-ops)
// ------------------------
STATIC mp_obj_t basalt_ui_screen_add(mp_obj_t self_in, mp_obj_t widget_in) {
    (void)self_in;
    if (s_item_count < BASALT_UI_MAX_ITEMS) {
        basalt_ui_item_t *it = &s_items[s_item_count++];
        it->obj = widget_in;
        if (mp_obj_is_type(widget_in, &basalt_ui_label_type)) {
            it->kind = BASALT_UI_ITEM_LABEL;
        } else if (mp_obj_is_type(widget_in, &basalt_ui_button_type)) {
            it->kind = BASALT_UI_ITEM_BUTTON;
        } else {
            it->kind = 0;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_screen_add_obj, basalt_ui_screen_add);

STATIC mp_obj_t basalt_ui_screen_show(mp_obj_t self_in) {
    (void)self_in;
    if (tft_console_is_ready()) {
        tft_console_clear();
    }
    for (int i = 0; i < s_item_count; ++i) {
        basalt_ui_item_t *it = &s_items[i];
        if (it->kind == BASALT_UI_ITEM_LABEL) {
            basalt_ui_label_t *lbl = MP_OBJ_TO_PTR(it->obj);
            const char *text = mp_obj_str_get_str(lbl->text);
            if (tft_console_is_ready()) {
                tft_console_write_at((int)lbl->x, (int)lbl->y, text);
            }
        } else if (it->kind == BASALT_UI_ITEM_BUTTON) {
            basalt_ui_button_t *btn = MP_OBJ_TO_PTR(it->obj);
            const char *text = mp_obj_str_get_str(btn->text);
            if (tft_console_is_ready()) {
                tft_console_write_at((int)btn->x, (int)btn->y, text);
            }
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_screen_show_obj, basalt_ui_screen_show);

STATIC mp_obj_t basalt_ui_screen_clear(mp_obj_t self_in) {
    (void)self_in;
    s_item_count = 0;
    if (tft_console_is_ready()) {
        tft_console_clear();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_screen_clear_obj, basalt_ui_screen_clear);

STATIC const mp_rom_map_elem_t basalt_ui_screen_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add),   MP_ROM_PTR(&basalt_ui_screen_add_obj)   },
    { MP_ROM_QSTR(MP_QSTR_show),  MP_ROM_PTR(&basalt_ui_screen_show_obj)  },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&basalt_ui_screen_clear_obj) },
};
STATIC MP_DEFINE_CONST_DICT(basalt_ui_screen_locals_dict, basalt_ui_screen_locals_dict_table);

STATIC mp_obj_t basalt_ui_screen_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    (void)n_args;
    (void)n_kw;
    (void)all_args;
    basalt_ui_screen_t *o = mp_obj_malloc(basalt_ui_screen_t, type);
    return MP_OBJ_FROM_PTR(o);
}

// Slot-based type (new MicroPython API)
MP_DEFINE_CONST_OBJ_TYPE(
    basalt_ui_screen_type,
    MP_QSTR_Screen,
    MP_TYPE_FLAG_NONE,
    make_new, basalt_ui_screen_make_new,
    locals_dict, &basalt_ui_screen_locals_dict
);

// ------------------------
// Button methods (no-ops)
// ------------------------
STATIC mp_obj_t basalt_ui_button_on_press(mp_obj_t self_in, mp_obj_t cb_in) {
    (void)self_in;
    (void)cb_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_button_on_press_obj, basalt_ui_button_on_press);

STATIC mp_obj_t basalt_ui_button_set_text(mp_obj_t self_in, mp_obj_t text_in) {
    basalt_ui_button_t *btn = MP_OBJ_TO_PTR(self_in);
    btn->text = text_in;
    if (tft_console_is_ready()) {
        const char *text = mp_obj_str_get_str(text_in);
        size_t new_len = strlen(text);
        size_t old_len = btn->last_len > 0 ? (size_t)btn->last_len : 0;
        if (old_len > new_len) {
            char tmp[96];
            size_t w = old_len;
            if (w >= sizeof(tmp)) w = sizeof(tmp) - 1;
            size_t cpy = new_len;
            if (cpy > w) cpy = w;
            memcpy(tmp, text, cpy);
            memset(tmp + cpy, ' ', w - cpy);
            tmp[w] = '\0';
            tft_console_write_at((int)btn->x, (int)btn->y, tmp);
        } else {
            tft_console_write_at((int)btn->x, (int)btn->y, text);
        }
        btn->last_len = (mp_int_t)new_len;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_button_set_text_obj, basalt_ui_button_set_text);

STATIC const mp_rom_map_elem_t basalt_ui_button_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on_press), MP_ROM_PTR(&basalt_ui_button_on_press_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_text), MP_ROM_PTR(&basalt_ui_button_set_text_obj) },
};
STATIC MP_DEFINE_CONST_DICT(basalt_ui_button_locals_dict, basalt_ui_button_locals_dict_table);

STATIC mp_obj_t basalt_ui_button_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_w,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_h,    MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args),
                              allowed_args, args);

    basalt_ui_button_t *o = mp_obj_malloc(basalt_ui_button_t, type);
    o->text = args[0].u_obj != MP_OBJ_NULL ? args[0].u_obj : mp_obj_new_str("", 0);
    o->last_len = (mp_int_t)strlen(mp_obj_str_get_str(o->text));
    o->x = args[1].u_int;
    o->y = args[2].u_int;
    o->w = args[3].u_int;
    o->h = args[4].u_int;
    return MP_OBJ_FROM_PTR(o);
}

MP_DEFINE_CONST_OBJ_TYPE(
    basalt_ui_button_type,
    MP_QSTR_Button,
    MP_TYPE_FLAG_NONE,
    make_new, basalt_ui_button_make_new,
    locals_dict, &basalt_ui_button_locals_dict
);

// ------------------------
// Label methods (no-ops)
// ------------------------
STATIC mp_obj_t basalt_ui_label_set_text(mp_obj_t self_in, mp_obj_t text_in) {
    basalt_ui_label_t *lbl = MP_OBJ_TO_PTR(self_in);
    lbl->text = text_in;
    if (tft_console_is_ready()) {
        const char *text = mp_obj_str_get_str(text_in);
        size_t new_len = strlen(text);
        size_t old_len = lbl->last_len > 0 ? (size_t)lbl->last_len : 0;
        if (old_len > new_len) {
            char tmp[96];
            size_t w = old_len;
            if (w >= sizeof(tmp)) w = sizeof(tmp) - 1;
            size_t cpy = new_len;
            if (cpy > w) cpy = w;
            memcpy(tmp, text, cpy);
            memset(tmp + cpy, ' ', w - cpy);
            tmp[w] = '\0';
            tft_console_write_at((int)lbl->x, (int)lbl->y, tmp);
        } else {
            tft_console_write_at((int)lbl->x, (int)lbl->y, text);
        }
        lbl->last_len = (mp_int_t)new_len;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_label_set_text_obj, basalt_ui_label_set_text);

STATIC const mp_rom_map_elem_t basalt_ui_label_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_text), MP_ROM_PTR(&basalt_ui_label_set_text_obj) },
};
STATIC MP_DEFINE_CONST_DICT(basalt_ui_label_locals_dict, basalt_ui_label_locals_dict_table);

STATIC mp_obj_t basalt_ui_label_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,    MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args),
                              allowed_args, args);

    basalt_ui_label_t *o = mp_obj_malloc(basalt_ui_label_t, type);
    o->text = args[0].u_obj != MP_OBJ_NULL ? args[0].u_obj : mp_obj_new_str("", 0);
    o->last_len = (mp_int_t)strlen(mp_obj_str_get_str(o->text));
    o->x = args[1].u_int;
    o->y = args[2].u_int;
    return MP_OBJ_FROM_PTR(o);
}

MP_DEFINE_CONST_OBJ_TYPE(
    basalt_ui_label_type,
    MP_QSTR_Label,
    MP_TYPE_FLAG_NONE,
    make_new, basalt_ui_label_make_new,
    locals_dict, &basalt_ui_label_locals_dict
);

// ------------------------
// Module-level factories
// ------------------------
STATIC mp_obj_t basalt_ui_screen_new(void) {
    return basalt_ui_screen_make_new(&basalt_ui_screen_type, 0, 0, NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_screen_obj, basalt_ui_screen_new);

STATIC mp_obj_t basalt_ui_button_new(size_t n_args, const mp_obj_t *args, mp_map_t *kw) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_w,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_h,    MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw,
                     MP_ARRAY_SIZE(allowed_args),
                     allowed_args, vals);

    basalt_ui_button_t *o = mp_obj_malloc(basalt_ui_button_t, &basalt_ui_button_type);
    o->text = vals[0].u_obj != MP_OBJ_NULL ? vals[0].u_obj : mp_obj_new_str("", 0);
    o->last_len = (mp_int_t)strlen(mp_obj_str_get_str(o->text));
    o->x = vals[1].u_int;
    o->y = vals[2].u_int;
    o->w = vals[3].u_int;
    o->h = vals[4].u_int;
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(basalt_ui_button_obj, 0, basalt_ui_button_new);

STATIC mp_obj_t basalt_ui_label_new(size_t n_args, const mp_obj_t *args, mp_map_t *kw) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,    MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw,
                     MP_ARRAY_SIZE(allowed_args),
                     allowed_args, vals);

    basalt_ui_label_t *o = mp_obj_malloc(basalt_ui_label_t, &basalt_ui_label_type);
    o->text = vals[0].u_obj != MP_OBJ_NULL ? vals[0].u_obj : mp_obj_new_str("", 0);
    o->last_len = (mp_int_t)strlen(mp_obj_str_get_str(o->text));
    o->x = vals[1].u_int;
    o->y = vals[2].u_int;
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(basalt_ui_label_obj, 0, basalt_ui_label_new);

STATIC mp_obj_t basalt_ui_set_title(mp_obj_t title_in) {
    (void)title_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_set_title_obj, basalt_ui_set_title);

// ------------------------
// Primitive draw/touch API
// ------------------------
STATIC mp_obj_t basalt_ui_ready(void) {
    return mp_obj_new_bool(tft_console_is_ready());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_ready_obj, basalt_ui_ready);

STATIC mp_obj_t basalt_ui_clear_fn(void) {
    if (tft_console_is_ready()) {
        tft_console_clear();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_clear_fn_obj, basalt_ui_clear_fn);

STATIC mp_obj_t basalt_ui_text(mp_obj_t msg_obj) {
    const char *msg = mp_obj_str_get_str(msg_obj);
    if (tft_console_is_ready() && msg) {
        tft_console_write(msg);
        tft_console_write("\n");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_text_obj, basalt_ui_text);

STATIC mp_obj_t basalt_ui_text_at(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t msg_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    const char *msg = mp_obj_str_get_str(msg_obj);
    if (tft_console_is_ready() && msg) {
        tft_console_write_at(x, y, msg);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(basalt_ui_text_at_obj, basalt_ui_text_at);

STATIC mp_obj_t basalt_ui_color(mp_obj_t color_obj) {
    int c = mp_obj_get_int(color_obj);
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    if (tft_console_is_ready()) {
        tft_console_set_color((uint16_t)c);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_color_obj, basalt_ui_color);

STATIC mp_obj_t basalt_ui_pixel(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t color_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int c = mp_obj_get_int(color_obj);
    if (c < 0) c = 0;
    if (c > 0xFFFF) c = 0xFFFF;
    tft_console_draw_pixel(x, y, (uint16_t)c);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(basalt_ui_pixel_obj, basalt_ui_pixel);

STATIC mp_obj_t basalt_ui_line(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_line_obj, 5, 5, basalt_ui_line);

STATIC mp_obj_t basalt_ui_rect(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_rect_obj, 5, 6, basalt_ui_rect);

STATIC mp_obj_t basalt_ui_circle(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_circle_obj, 4, 5, basalt_ui_circle);

STATIC mp_obj_t basalt_ui_ellipse(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(basalt_ui_ellipse_obj, 5, 6, basalt_ui_ellipse);

STATIC mp_obj_t basalt_ui_touch(void) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_touch_obj, basalt_ui_touch);

// ------------------------
// Export into module dict
// ------------------------
void basalt_ui_init(mp_obj_module_t *ui_mod) {
    mp_obj_t g = MP_OBJ_FROM_PTR(ui_mod->globals);

    // Factory functions (lowercase)
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_screen),    MP_OBJ_FROM_PTR(&basalt_ui_screen_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_button),    MP_OBJ_FROM_PTR(&basalt_ui_button_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_label),     MP_OBJ_FROM_PTR(&basalt_ui_label_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_set_title), MP_OBJ_FROM_PTR(&basalt_ui_set_title_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("ready")),   MP_OBJ_FROM_PTR(&basalt_ui_ready_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("clear")),   MP_OBJ_FROM_PTR(&basalt_ui_clear_fn_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("text")),    MP_OBJ_FROM_PTR(&basalt_ui_text_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("text_at")), MP_OBJ_FROM_PTR(&basalt_ui_text_at_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("color")),   MP_OBJ_FROM_PTR(&basalt_ui_color_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("pixel")),   MP_OBJ_FROM_PTR(&basalt_ui_pixel_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("line")),    MP_OBJ_FROM_PTR(&basalt_ui_line_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("rect")),    MP_OBJ_FROM_PTR(&basalt_ui_rect_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("circle")),  MP_OBJ_FROM_PTR(&basalt_ui_circle_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("ellipse")), MP_OBJ_FROM_PTR(&basalt_ui_ellipse_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(qstr_from_str("touch")),   MP_OBJ_FROM_PTR(&basalt_ui_touch_obj));

    // Types (uppercase)
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Screen), MP_OBJ_FROM_PTR(&basalt_ui_screen_type));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Button), MP_OBJ_FROM_PTR(&basalt_ui_button_type));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Label),  MP_OBJ_FROM_PTR(&basalt_ui_label_type));
}
