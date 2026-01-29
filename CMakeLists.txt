cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/runtime/python
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(basaltos)

# Generate SPIFFS image from ./spiffs and flash into "storage" partition
spiffs_create_partition_image(storage spiffs FLASH_IN_PROJECT)
