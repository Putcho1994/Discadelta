cmake_minimum_required(VERSION 3.31.6)

find_package(Vulkan QUIET)

if(Vulkan_FOUND)
    message(STATUS "Vulkan found → version ${Vulkan_VERSION}")
    message(STATUS "  include: ${Vulkan_INCLUDE_DIR}")
    message(STATUS "  library: ${Vulkan_LIBRARY}")
else()
    message(WARNING "
                  Vulkan SDK NOT FOUND
 → No Vulkan support will be built
 → GUI / rendering features will be disabled or stubbed

 To enable Vulkan:
  • Install Vulkan SDK → https://vulkan.lunarg.com
  • Set VULKAN_SDK environment variable
  • Re-run cmake
")
    # You can define a variable / option here
    set(HAS_VULKAN_SUPPORT OFF CACHE INTERNAL "Vulkan support status")
endif()

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
