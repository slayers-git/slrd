cmake_minimum_required (VERSION 3.12)
project (slrdcompute)

include (${CMAKE_SOURCE_DIR}/cmake/add_shaders.cmake)

set (SRC
    src/compute.cpp
    src/main.cpp)

set (GLSL_SRC
    ${PROJECT_SOURCE_DIR}/src/shaders/particle.comp
    ${PROJECT_SOURCE_DIR}/src/shaders/simple.vert
    ${PROJECT_SOURCE_DIR}/src/shaders/simple.frag)

message (${SLRD_FRAMEWORK_INCLUDE})

add_executable (slrdcompute ${SRC})
target_link_libraries (slrdcompute slrd::framework)
target_include_directories (slrdcompute PRIVATE ${SLRD_FRAMEWORK_INCLUDE})

add_shaders (compshaders ${GLSL_SRC})
add_dependencies (slrdcompute compshaders)
