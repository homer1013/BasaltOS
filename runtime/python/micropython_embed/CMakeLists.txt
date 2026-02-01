file(GLOB_RECURSE MPY_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

list(APPEND MPY_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/port/modui.c"
)

idf_component_register(
    SRCS ${MPY_SRCS}
    INCLUDE_DIRS "." "port" "py" "extmod" "shared/runtime" "genhdr" "${PROJECT_DIR}/config/generated"
    REQUIRES driver freertos
    PRIV_REQUIRES main
)

# Reduce warnings from upstream MicroPython sources
set_source_files_properties(${MPY_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-unused-parameter -Wno-unused-function")
