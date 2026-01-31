cmake_minimum_required(VERSION 3.16)

# Use generated sdkconfig defaults from BasaltOS configurator
set(SDKCONFIG_DEFAULTS
    ${CMAKE_SOURCE_DIR}/config/generated/sdkconfig.defaults
)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/runtime/python/micropython_embed
    ${CMAKE_SOURCE_DIR}/basalt_hal
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(basaltos)

# Make generated BasaltOS config header globally visible
idf_build_set_property(
    INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/config/generated
    APPEND
)

# Generate SPIFFS image from ./spiffs and flash into "storage" partition
spiffs_create_partition_image(storage spiffs FLASH_IN_PROJECT)
