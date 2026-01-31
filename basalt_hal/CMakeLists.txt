idf_component_register(
    INCLUDE_DIRS
        "include"
)

if(IDF_TARGET STREQUAL "esp32")
    set(HAL_PORT_SRCS
        "ports/esp32/hal_gpio.c"
        "ports/esp32/hal_i2c.c"
        "ports/esp32/hal_uart.c"
    )
else()
    message(FATAL_ERROR "Basalt HAL: unsupported IDF_TARGET=${IDF_TARGET}. Add a port under basalt_hal/ports/<target>/")
endif()

target_sources(${COMPONENT_LIB} PRIVATE ${HAL_PORT_SRCS})
