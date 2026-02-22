cmake_minimum_required(VERSION 3.31.6)
project(Discadelta LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_subdirectory(src)

find_package(Vulkan-Headers QUIET)

option(HAS_VULKAN "Vulkan support" OFF)

if(Vulkan-Headers_FOUND)
    message(STATUS "Discadelta: Vulkan Headers - found")
    set(HAS_VULKAN ON)
else()
    find_package(Vulkan QUIET)

    if(Vulkan_FOUND)
        message(STATUS "Discadelta: Vulkan - found → version ${Vulkan_VERSION}")
        message(STATUS "Discadelta: Vulkan - include: ${Vulkan_INCLUDE_DIR}")
        message(STATUS "Discadelta: Vulkan - library: ${Vulkan_LIBRARY}")
        target_link_libraries(src PUBLIC Vulkan::Vulkan)
        set(HAS_VULKAN ON)
    else()
    message(WARNING "
           Discadelta: Vulkan SDK NOT FOUND
 → No Vulkan support will be built
 → GUI / rendering features will be disabled or stubbed

 To enable Vulkan:
  • Install Vulkan SDK → https://vulkan.lunarg.com
  • Set VULKAN_SDK environment variable
  • Re-run cmake
")
    SET(HAS_VULKAN OFF)
    endif ()
endif()

if(HAS_VULKAN)
    target_compile_definitions(src PRIVATE HAS_VULKAN=1)
    message(STATUS "Discadelta: Enabled Vulkan features")
endif()

# Option to build samples (default ON)
option(DISCADELTA_SAMPLES "Build sample executables" ON)

if(DISCADELTA_SAMPLES)
    message(STATUS "Discadelta: Building samples")
    add_executable(ch01_basic       samples/chapter_01_sample.cpp)
    add_executable(ch02_constraints samples/chapter_02_sample.cpp)
    add_executable(ch03_optimized   samples/chapter_03_sample.cpp)
    add_executable(linear_sample samples/linear_sample.cpp)
    add_executable(rect_sample samples/rect_sample.cpp)
    # Link chapter_04 to core (others are standalone)
    target_link_libraries(linear_sample PRIVATE src)
    target_link_libraries(rect_sample PRIVATE src)
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

