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
// Export into module dict
// ------------------------
void basalt_ui_init(mp_obj_module_t *ui_mod) {
    mp_obj_t g = MP_OBJ_FROM_PTR(ui_mod->globals);

    // Factory functions (lowercase)
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_screen),    MP_OBJ_FROM_PTR(&basalt_ui_screen_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_button),    MP_OBJ_FROM_PTR(&basalt_ui_button_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_label),     MP_OBJ_FROM_PTR(&basalt_ui_label_obj));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_set_title), MP_OBJ_FROM_PTR(&basalt_ui_set_title_obj));

    // Types (uppercase)
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Screen), MP_OBJ_FROM_PTR(&basalt_ui_screen_type));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Button), MP_OBJ_FROM_PTR(&basalt_ui_button_type));
    mp_obj_dict_store(g, MP_OBJ_NEW_QSTR(MP_QSTR_Label),  MP_OBJ_FROM_PTR(&basalt_ui_label_type));
}
