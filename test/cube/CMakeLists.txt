cmake_minimum_required (VERSION 3.12)
project (slrdcube)

include (${CMAKE_SOURCE_DIR}/cmake/add_shaders.cmake)

set (SRC
    src/cube.cpp)

set (GLSL_SRC
    ${PROJECT_SOURCE_DIR}/src/shaders/cube.vert
    ${PROJECT_SOURCE_DIR}/src/shaders/cube.frag)

add_executable (slrdcube ${SRC})

find_package (SDL2 REQUIRED)
target_link_libraries (slrdcube
    ${SDL2_LIBRARIES}
    SLRD::SLRD)

add_shaders (cubeshaders ${GLSL_SRC})
add_dependencies (slrdcube cubeshaders)

add_custom_command (TARGET slrdcube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:slrdcube>/res")
