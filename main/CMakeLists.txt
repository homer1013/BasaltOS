idf_component_register(SRCS "app_main.c" "tft_console.c" "mpy_runtime.c"
                    INCLUDE_DIRS "."
                    REQUIRES micropython_embed vfs spiffs nvs_flash fatfs sdmmc)
