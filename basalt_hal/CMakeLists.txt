# Basalt HAL component
# Build the correct port sources for the active IDF target.

set(HAL_PORT_SRCS "")

if(IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32c3")
    set(PORT_DIR "ports/esp32")
    list(APPEND HAL_PORT_SRCS
        "${PORT_DIR}/hal_gpio.c"
        "${PORT_DIR}/hal_uart.c"
        "${PORT_DIR}/hal_i2c.c"
    )
else()
    message(FATAL_ERROR "Basalt HAL: unsupported IDF_TARGET='${IDF_TARGET}'")
endif()

idf_component_register(
    SRCS ${HAL_PORT_SRCS}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS ${PORT_DIR}
    REQUIRES driver
)
