idf_component_register(
    SRCS
        "app_main.c"
        "smoke_test.c"
        "tft_console.c"
        "mpy_runtime.c"
        "runtime_dispatch.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash spiffs console micropython_embed basalt_hal
    PRIV_REQUIRES fatfs esp_wifi esp_event esp_netif bt esp_driver_usb_serial_jtag esp_driver_ledc
)
