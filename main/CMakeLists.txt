set(BASALT_MAIN_REQUIRES nvs_flash spiffs console micropython_embed basalt_hal)
set(BASALT_MAIN_PRIV_REQUIRES fatfs esp_wifi esp_event esp_netif bt esp_driver_usb_serial_jtag esp_driver_ledc)
if(BASALT_ENABLE_LUA_RUNTIME)
    list(APPEND BASALT_MAIN_PRIV_REQUIRES lua_embed)
endif()

idf_component_register(
    SRCS
        "app_main.c"
        "smoke_test.c"
        "tft_console.c"
        "mpy_runtime.c"
        "lua_runtime.c"
        "runtime_dispatch.c"
    INCLUDE_DIRS "."
    REQUIRES ${BASALT_MAIN_REQUIRES}
    PRIV_REQUIRES ${BASALT_MAIN_PRIV_REQUIRES}
)

if(BASALT_ENABLE_LUA_RUNTIME)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BASALT_HAS_LUA_EMBED=1)
    target_link_libraries(${COMPONENT_LIB} PRIVATE idf::lua_embed)
endif()
