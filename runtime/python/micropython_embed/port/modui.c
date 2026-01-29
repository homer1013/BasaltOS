#include "py/obj.h"
#include "py/runtime.h"
#include "py/argcheck.h"

#include "modui.h"

typedef struct _basalt_ui_screen_t {
    mp_obj_base_t base;
} basalt_ui_screen_t;

typedef struct _basalt_ui_button_t {
    mp_obj_base_t base;
} basalt_ui_button_t;

typedef struct _basalt_ui_label_t {
    mp_obj_base_t base;
} basalt_ui_label_t;

// Screen methods (no-ops)
STATIC mp_obj_t basalt_ui_screen_add(mp_obj_t self_in, mp_obj_t widget_in) {
    (void)self_in;
    (void)widget_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_screen_add_obj, basalt_ui_screen_add);

STATIC mp_obj_t basalt_ui_screen_show(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_screen_show_obj, basalt_ui_screen_show);

STATIC mp_obj_t basalt_ui_screen_clear(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_screen_clear_obj, basalt_ui_screen_clear);

STATIC const mp_rom_map_elem_t basalt_ui_screen_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&basalt_ui_screen_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&basalt_ui_screen_show_obj) },
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

STATIC const mp_obj_type_t basalt_ui_screen_type = {
    { &mp_type_type },
    .name = MP_QSTR_Screen,
    .make_new = basalt_ui_screen_make_new,
    .locals_dict = (mp_obj_dict_t *)&basalt_ui_screen_locals_dict,
};

// Button methods (no-ops)
STATIC mp_obj_t basalt_ui_button_on_press(mp_obj_t self_in, mp_obj_t cb_in) {
    (void)self_in;
    (void)cb_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(basalt_ui_button_on_press_obj, basalt_ui_button_on_press);

STATIC mp_obj_t basalt_ui_button_set_text(mp_obj_t self_in, mp_obj_t text_in) {
    (void)self_in;
    (void)text_in;
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
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_w, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_h, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    (void)args;
    basalt_ui_button_t *o = mp_obj_malloc(basalt_ui_button_t, type);
    return MP_OBJ_FROM_PTR(o);
}

STATIC const mp_obj_type_t basalt_ui_button_type = {
    { &mp_type_type },
    .name = MP_QSTR_Button,
    .make_new = basalt_ui_button_make_new,
    .locals_dict = (mp_obj_dict_t *)&basalt_ui_button_locals_dict,
};

// Label methods (no-ops)
STATIC mp_obj_t basalt_ui_label_set_text(mp_obj_t self_in, mp_obj_t text_in) {
    (void)self_in;
    (void)text_in;
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
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    (void)args;
    basalt_ui_label_t *o = mp_obj_malloc(basalt_ui_label_t, type);
    return MP_OBJ_FROM_PTR(o);
}

STATIC const mp_obj_type_t basalt_ui_label_type = {
    { &mp_type_type },
    .name = MP_QSTR_Label,
    .make_new = basalt_ui_label_make_new,
    .locals_dict = (mp_obj_dict_t *)&basalt_ui_label_locals_dict,
};

// Module helpers
STATIC mp_obj_t basalt_ui_screen_new(void) {
    return basalt_ui_screen_make_new(&basalt_ui_screen_type, 0, 0, NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(basalt_ui_screen_obj, basalt_ui_screen_new);

STATIC mp_obj_t basalt_ui_button_new(size_t n_args, const mp_obj_t *args, mp_map_t *kw) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_w, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_h, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
    (void)vals;
    basalt_ui_button_t *o = mp_obj_malloc(basalt_ui_button_t, &basalt_ui_button_type);
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(basalt_ui_button_obj, 1, basalt_ui_button_new);

STATIC mp_obj_t basalt_ui_label_new(size_t n_args, const mp_obj_t *args, mp_map_t *kw) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
    (void)vals;
    basalt_ui_label_t *o = mp_obj_malloc(basalt_ui_label_t, &basalt_ui_label_type);
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(basalt_ui_label_obj, 1, basalt_ui_label_new);

STATIC mp_obj_t basalt_ui_set_title(mp_obj_t title_in) {
    (void)title_in;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(basalt_ui_set_title_obj, basalt_ui_set_title);

void basalt_ui_init(mp_obj_module_t *ui_mod) {
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("screen")),
        MP_OBJ_FROM_PTR(&basalt_ui_screen_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("button")),
        MP_OBJ_FROM_PTR(&basalt_ui_button_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("label")),
        MP_OBJ_FROM_PTR(&basalt_ui_label_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("set_title")),
        MP_OBJ_FROM_PTR(&basalt_ui_set_title_obj));

    // Expose types for future use
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("Screen")),
        MP_OBJ_FROM_PTR(&basalt_ui_screen_type));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("Button")),
        MP_OBJ_FROM_PTR(&basalt_ui_button_type));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(ui_mod->globals), MP_OBJ_NEW_QSTR(qstr_from_str("Label")),
        MP_OBJ_FROM_PTR(&basalt_ui_label_type));
}
