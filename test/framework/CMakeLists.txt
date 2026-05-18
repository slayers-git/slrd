cmake_minimum_required (VERSION 3.12)
project (slrd_framework)

set (SRC
    src/app.cpp
    src/camera.cpp
    src/profiler.cpp
    src/util.cpp)

find_package (Vulkan REQUIRED)
find_package (SDL2 REQUIRED)

add_library (slrdframework ${SRC})
add_library (slrd::framework ALIAS slrdframework)

include_directories (${PROJECT_SOURCE_DIR}/include)

if (NOT IMGUI_PATH)
    set (IMGUI_PATH "${CMAKE_SOURCE_DIR}/vendor/imgui"
        CACHE PATH "ImGui path")
endif ()

set (IMGUI_SRC
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp)

add_library (imgui STATIC ${IMGUI_SRC})
target_include_directories (imgui PRIVATE
    ${IMGUI_PATH}
    ${SDL2_INCLUDE_DIRS})

target_link_libraries (slrdframework Vulkan::Vulkan
    slrd::slrd
    ${SDL2_LIBRARIES}
    imgui)

include_directories (${PROJECT_SOURCE_DIR}/include)
include_directories (${SDL2_INCLUDE_DIRS} ${IMGUI_PATH})

set (SLRD_FRAMEWORK_INCLUDE
    ${PROJECT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_PATH} PARENT_SCOPE)
