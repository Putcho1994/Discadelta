cmake_minimum_required(VERSION 3.31.6)

add_library(src STATIC)

# Core library (interface for modules)
# add_library(discadelta_core OBJECT)
target_sources(src
        PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ufox_discadelta_lib.cppm
        ufox_discadelta_core.cppm
)

target_include_directories(src PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
