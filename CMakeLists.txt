cmake_minimum_required(VERSION 3.31.6)
project(Discadelta LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_subdirectory(src)



# Option to build samples (default ON)
option(DISCADELTA_SAMPLES "Build sample executables" ON)

if(DISCADELTA_SAMPLES)
    message(STATUS "Building Discadelta samples")
    add_executable(ch01_basic       samples/chapter_01_sample.cpp)
    add_executable(ch02_constraints samples/chapter_02_sample.cpp)
    add_executable(ch03_optimized   samples/chapter_03_sample.cpp)
    add_executable(ch04_placing     samples/chapter_04_sample.cpp)
    add_executable(ch05_nesting     samples/chapter_05_sample.cpp)
    # Link chapter_04 to core (others are standalone)
    target_link_libraries(ch04_placing PRIVATE src)
    target_link_libraries(ch05_nesting PRIVATE src)
endif()

# === Installation ===
include(GNUInstallDirs)

install(TARGETS src
        EXPORT DiscadeltaTargets
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ufox/geometry/discadelta
)

install(EXPORT DiscadeltaTargets
        FILE DiscadeltaTargets.cmake
        NAMESPACE Discadelta::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Discadelta
)


# Package config
include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/DiscadeltaConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DiscadeltaConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Discadelta
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DiscadeltaConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Discadelta
)

