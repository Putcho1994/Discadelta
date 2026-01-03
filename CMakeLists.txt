cmake_minimum_required(VERSION 4.2)
project(Discadelta LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Core library (interface for modules)
add_library(discadelta_core OBJECT)
target_sources(discadelta_core
        PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
        core/ufox_discadelta_lib.cppm
        core/ufox_discadelta_core.cppm
)

# Option to build samples (default ON)
option(DISCADelta_BUILD_SAMPLES "Build sample executables" ON)

if(DISCADelta_BUILD_SAMPLES)
    message(STATUS "Building Discadelta samples")
    add_executable(ch01_basic       samples/chapter_01_sample.cpp)
    add_executable(ch02_constraints samples/chapter_02_sample.cpp)
    add_executable(ch03_optimized   samples/chapter_03_sample.cpp)
    add_executable(ch04_placing     samples/chapter_04_sample.cpp)

    # Link chapter_04 to core (others are standalone)
    target_link_libraries(ch04_placing PRIVATE discadelta_core)
endif()

# Installation for CPM/package use
install(TARGETS discadelta_core
        EXPORT DiscadeltaTargets
        FILE_SET CXX_MODULES DESTINATION include/ufox/geometry/discadelta
)

install(EXPORT DiscadeltaTargets
        FILE DiscadeltaTargets.cmake
        NAMESPACE Discadelta::
        DESTINATION lib/cmake/Discadelta
)

# Generate package config
include(CMakePackageConfigHelpers)
configure_package_config_file(
        DiscadeltaConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DiscadeltaConfig.cmake
        INSTALL_DESTINATION lib/cmake/Discadelta
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/DiscadeltaConfig.cmake
        DESTINATION lib/cmake/Discadelta
)