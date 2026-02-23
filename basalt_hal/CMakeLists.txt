# Basalt HAL component
# Build the correct port sources for the active IDF target.

set(HAL_PORT_SRCS "")

if(IDF_TARGET STREQUAL "esp32")
    set(PORT_DIR "ports/esp32")
elseif(IDF_TARGET STREQUAL "esp32c3")
    set(PORT_DIR "ports/esp32c3")
elseif(IDF_TARGET STREQUAL "esp32c6")
    set(PORT_DIR "ports/esp32c6")
elseif(IDF_TARGET STREQUAL "esp32s3")
    set(PORT_DIR "ports/esp32s3")
endif()

if(PORT_DIR)
    list(APPEND HAL_PORT_SRCS
        "${PORT_DIR}/hal_gpio.c"
        "${PORT_DIR}/hal_uart.c"
        "${PORT_DIR}/hal_i2c.c"
        "${PORT_DIR}/hal_spi.c"
        "${PORT_DIR}/hal_timer.c"
        "${PORT_DIR}/hal_pwm.c"
        "${PORT_DIR}/hal_adc.c"
        "${PORT_DIR}/hal_i2s.c"
        "${PORT_DIR}/hal_rmt.c"
    )
else()
    message(FATAL_ERROR "Basalt HAL: unsupported IDF_TARGET='${IDF_TARGET}'")
endif()

idf_component_register(
    SRCS ${HAL_PORT_SRCS}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS ${PORT_DIR}
    REQUIRES esp_driver_gpio esp_driver_uart esp_driver_i2c esp_driver_spi esp_driver_ledc esp_adc esp_driver_i2s esp_driver_rmt esp_timer driver
)
