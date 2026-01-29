// Basalt OS MicroPython embed config
#include <port/mpconfigport_common.h>

#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)
#define MICROPY_PY_SYS                          (1)
#define MICROPY_PY_USR_C_MODULES                (1)

#define MICROPY_PY_SYS_PLATFORM                 "BasaltOS-ESP32"
#define MICROPY_GCREGS_SETJMP                   (1)
#define MICROPY_NLR_SETJMP                      (1)
#define MICROPY_PY_BUILTINS_HELP                (0)
#define MICROPY_REPL_INFO                       (0)
