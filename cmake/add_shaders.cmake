function (add_shaders TARGET_NAME)
    set (_GLSL_SRC ${ARGN})

    find_program (GLSL_COMPILER "glslangValidator" REQUIRED)
    foreach (file ${_GLSL_SRC})
        get_filename_component (fname ${file} NAME)

        message ("Compiling " ${file} " shader")
        set (SPIRV_OUTPUT "${PROJECT_BINARY_DIR}/shaders/${fname}.spv")

        add_custom_command (OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${PROJECT_BINARY_DIR}/shaders"
            COMMAND ${GLSL_COMPILER} -V ${file} -o ${SPIRV_OUTPUT}
            DEPENDS ${file})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})
    endforeach(file)

    add_custom_target (${TARGET_NAME} DEPENDS
        ${SPIRV_BINARY_FILES})
endfunction ()

