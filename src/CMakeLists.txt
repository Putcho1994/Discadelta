cmake_minimum_required(VERSION 3.31.6)

find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR
            "Vulkan SDK not found.\n"
            "Please install Vulkan SDK from https://vulkan.lunarg.com/\n"
            "and set environment variable VULKAN_SDK to the install folder."
    )
endif()

# Debug: show exactly where CMake found Vulkan
message(STATUS "")
message(STATUS "Vulkan information:")
message(STATUS "  Vulkan_FOUND       = ${Vulkan_FOUND}")
message(STATUS "  Vulkan_VERSION     = ${Vulkan_VERSION}")
message(STATUS "  Vulkan_INCLUDE_DIR = ${Vulkan_INCLUDE_DIR}")
message(STATUS "  Vulkan_LIBRARY     = ${Vulkan_LIBRARY}")
message(STATUS "  Imported target    = Vulkan::Vulkan")
message(STATUS "")

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
