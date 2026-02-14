cmake_minimum_required(VERSION 3.16)

# Layer project defaults first (partitions, console, etc.), then generated defaults.
# This keeps required base settings (e.g., SPIFFS storage partition) while allowing
# configurator output to override feature-specific settings.
set(SDKCONFIG_DEFAULTS
    ${CMAKE_SOURCE_DIR}/sdkconfig.defaults
)

if(EXISTS ${CMAKE_SOURCE_DIR}/config/generated/sdkconfig.defaults)
    list(APPEND SDKCONFIG_DEFAULTS
        ${CMAKE_SOURCE_DIR}/config/generated/sdkconfig.defaults
    )
endif()

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/runtime/python/micropython_embed
    ${CMAKE_SOURCE_DIR}/basalt_hal
)

option(BASALT_ENABLE_LUA_RUNTIME "Enable Lua runtime skeleton component" OFF)
if(BASALT_ENABLE_LUA_RUNTIME)
    list(APPEND EXTRA_COMPONENT_DIRS
        ${CMAKE_SOURCE_DIR}/runtime/lua/lua_embed
    )
endif()

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
