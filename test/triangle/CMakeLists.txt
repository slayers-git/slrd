cmake_minimum_required (VERSION 3.12)
project (slrdtriangle)

include (${CMAKE_SOURCE_DIR}/cmake/add_shaders.cmake)

set (SRC
    src/triangle.cpp)

set (GLSL_SRC
    ${PROJECT_SOURCE_DIR}/src/shaders/basic.vert
    ${PROJECT_SOURCE_DIR}/src/shaders/basic.frag)

add_executable (slrdtriangle ${SRC})

find_package (SDL2 REQUIRED)
target_link_libraries (slrdtriangle
    ${SDL2_LIBRARIES}
    slrd::slrd)

add_shaders (triangleshaders ${GLSL_SRC})
add_dependencies (slrdtriangle triangleshaders)
