# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
#                    INCLUDE_DIRS "includes"
                    REQUIRES "core2forAWS" "esp-cryptoauthlib" "nvs_flash")
