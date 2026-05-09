# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required (VERSION 3.20)
project (slrd VERSION 0.0.1)

option (SLRD_DEBUG "Enable debug assertions" FALSE)
option (SLRD_VULKAN_ENABLED "Enable vulkan backend" TRUE)
option (SLRD_VULKAN_DEBUG "Enable vulkan debug layers" TRUE)
option (SLRD_TESTS "Compile tests" FALSE)
option (SLRD_INSTALL "Install slrd" FALSE)

option (SLRD_REQUIRE_DEBUG_NAMES
    "Enable debug extensions loading, if required for object naming"
    ON)

option (SLRD_RESOURCE_PROFILER "Enable the resource usage profiler" ON)

# Bundled libraries
option (SLRD_USE_VENDOR_VMA "Use bundled VMA" ON)
option (SLRD_USE_VENDOR_SPIRV_REFLECT "Use bundled SPIRV-Reflect" ON)

set (SLRD_VMA_PATH "" CACHE PATH "Path to VulkanMemoryAllocator")
set (SLRD_SPIRV_REFLECT_PATH "" CACHE PATH "Path to SPIRV-Reflect")

set (SLRD_VERSION VERSION ${CMAKE_PROJECT_VERSION})

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)

set (SRC src/api.cpp
         src/device.cpp
         src/profiler.cpp
         src/error.cpp
         src/surface.cpp
         src/uniformset.cpp)

set (VULKAN_SRC
         src/vulkan/api.cpp
         src/vulkan/device.cpp
         src/vulkan/surface.cpp
         src/vulkan/swapchain.cpp
         src/vulkan/shader.cpp
         src/vulkan/pipelinelayout.cpp
         src/vulkan/pipeline.cpp
         src/vulkan/buffer.cpp
         src/vulkan/texture.cpp
         src/vulkan/commandqueue.cpp
         src/vulkan/commandbuffer.cpp
         src/vulkan/renderpass.cpp
         src/vulkan/fence.cpp
         src/vulkan/sampler.cpp
         src/vulkan/uniformset.cpp
         src/vulkan/descpoolmanager.cpp
         src/vulkan/pipelinemanager.cpp)

if (SLRD_VULKAN_ENABLED)
    message ("Compiling with Vulkan support")

    if (SLRD_VULKAN_DEBUG)
        set (SLRD_VULKAN_DEBUG_MESSENGER_ENABLED TRUE)
    endif ()
endif ()

if (SLRD_DEBUG)
    message ("SLRD debug is enabled!")
endif ()

configure_file (src/config.h.in
    ${PROJECT_SOURCE_DIR}/include/slrd/config.hpp @ONLY)

add_library (slrd ${SRC})
add_library (slrd::slrd ALIAS slrd)

target_include_directories (slrd PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src> 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_precompile_headers (slrd PRIVATE ${PCH})

if (SLRD_VULKAN_ENABLED)
    if (SLRD_USE_VENDOR_VMA)
        message ("Using vendor-provided VMA")
        set (SLRD_VMA_PATH "${CMAKE_SOURCE_DIR}/vendor/VulkanMemoryAllocator/include")
    else ()
        if (NOT SLRD_VMA_PATH)
            message (FATAL_ERROR
                "Build is set to not use the vendor-provided VMA, but no user-defined path was provided")
        endif ()
    endif ()

    if (NOT IS_DIRECTORY ${SLRD_VMA_PATH})
        message (FATAL_ERROR "SLRD_VMA_PATH (${SLRD_VMA_PATH}) doesn't exist!")
    endif ()

    if (SLRD_USE_VENDOR_SPIRV_REFLECT)
        message ("Using vendor-provided SPIRV-Reflect")
        set (SLRD_SPIRV_REFLECT_PATH "${CMAKE_SOURCE_DIR}/vendor/SPIRV-Reflect")
    else ()
        if (NOT SLRD_SPIRV_REFLECT_PATH)
            message (FATAL_ERROR
                "Build is set to not use the vendor-provided SPIRV-Reflect, but no user-defined path was provided")
        endif ()
    endif ()

    if (NOT IS_DIRECTORY ${SLRD_SPIRV_REFLECT_PATH})
        message (FATAL_ERROR "SLRD_SPIRV_REFLECT_PATH (${SLRD_SPIRV_REFLECT_PATH}) doesn't exist!")
    endif ()

    find_package (Vulkan REQUIRED)

    target_include_directories (slrd PRIVATE
        $<BUILD_INTERFACE:${SLRD_SPIRV_REFLECT_PATH}>
        $<BUILD_INTERFACE:${SLRD_VMA_PATH}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/vendor/rocket>
    )
    # include_directories (vendor/rocket)

    add_library (vulkan_memory_allocator OBJECT src/vulkan/vma.cpp)
    target_include_directories (vulkan_memory_allocator PRIVATE ${SLRD_VMA_PATH})
    target_compile_options (vulkan_memory_allocator
        PRIVATE -Wno-nullability-completeness)

    add_library (xxhash OBJECT vendor/xxHash/xxhash.c)
    target_include_directories (xxhash PRIVATE vendor/xxHash)

    add_library (spirv_reflect OBJECT ${SLRD_SPIRV_REFLECT_PATH}/spirv_reflect.c)
    target_include_directories (spirv_reflect PRIVATE ${SLRD_VMA_PATH})

    target_sources (slrd PRIVATE
        ${VULKAN_SRC}
        $<TARGET_OBJECTS:xxhash>
        $<TARGET_OBJECTS:spirv_reflect>
        $<TARGET_OBJECTS:vulkan_memory_allocator>
    )
    target_link_libraries (slrd
        PUBLIC Vulkan::Vulkan
    )
endif ()

if (SLRD_TESTS)
    add_subdirectory (test)
endif ()

if (SLRD_INSTALL)
    include (GNUInstallDirs)
    include (CMakePackageConfigHelpers)

    configure_package_config_file (
        "cmake/slrd-config.cmake.in"
        "slrd-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/slrd"
    )

    write_basic_package_version_file (
        "slrd-config-version.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install (
        TARGETS slrd
        EXPORT slrdTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install (
        DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    install (
        EXPORT slrdTargets
        FILE slrd-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/slrd
        NAMESPACE slrd::
    )

    install (FILES
        "${CMAKE_CURRENT_BINARY_DIR}/slrd-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/slrd-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/slrd"
    )
endif ()
