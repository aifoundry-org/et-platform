#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

function(jinja_generate_tf_protocol SRCS HDRS)
    if(NOT ARGN)
        message(SEND_ERROR "Error: jinja_generate_tf_protocol() called without any jinja files")
        return()
    endif()

    set(options )
    set(oneValueArgs TARGET TF_PROTOCOL_CODEGEN_DIR)
    set(multiValueArgs JINJA_TEMPLATES)
    cmake_parse_arguments(JINJA_GENERATE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT JINJA_GENERATE_TARGET)
        message(SEND_ERROR "Error: jinja_generate_tf_protocol() called without argument: TARGET")
        return()
    endif()

    if (NOT JINJA_GENERATE_TF_PROTOCOL_CODEGEN_DIR)
        message(SEND_ERROR "Error: jinja_generate_api_cpp() called without argument: TF_PROTOCOL_CODEGEN_DIR")
        return()
    endif()

    if (NOT JINJA_GENERATE_JINJA_TEMPLATES)
        message(SEND_ERROR "Error: jinja_generate_tf_protocol() called without argument: JINJA_TEMPLATES")
        return()
    endif()

    set(TF_SPEC_FILE ${JINJA_GENERATE_TF_PROTOCOL_CODEGEN_DIR}/tf_specification.json)

    set(${SRCS})
    set(${HDRS})
    foreach(FIL ${JINJA_GENERATE_JINJA_TEMPLATES})
        get_filename_component(FIL_ABS  ${FIL} ABSOLUTE)  # file name absolute path
        get_filename_component(FIL_EXT  ${FIL} LAST_EXT)  # file name last extension (.c from d/a.b.c)
        get_filename_component(FIL_DIR  ${FIL} DIRECTORY) # Directory without file name
        get_filename_component(FIL_WE   ${FIL} NAME_WE)   # file name without directory or longest extension (a from d/a.b.c)
        get_filename_component(FIL_WLE  ${FIL} NAME_WLE)  # file name without directory or last extension (a.b from d/a.b.c)
        get_filename_component(SRC_EXT  ${FIL_WLE} LAST_EXT) # extract '.h' or '.cc/.cpp' from name

        if (NOT FIL_EXT STREQUAL ".jinja")
            message(SEND_ERROR "Error: jinja_generate_tf_protocol() called with file without jinja extension (${FIL_EXT}). Abort")
            return()
        endif()

        set(JINJA_TEMPLATE      ${FIL_ABS})

        string(REPLACE "_" ";" FIL_LIST "${FIL_WE}${SRC_EXT}")
        string(REPLACE ";" "_" GENERATED_FILE_NAME "${FIL_LIST}")

        set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/esperanto")
        set(GENERATED_FILE "${GENERATED_DIR}/${GENERATED_FILE_NAME}")

        if (SRC_EXT MATCHES ".h")
            list(APPEND ${HDRS} ${GENERATED_FILE})
        else()
            list(APPEND ${SRCS} ${GENERATED_FILE})
        endif()

        add_custom_command(
            OUTPUT  ${GENERATED_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
            COMMAND ${JINJA_GENERATE_TF_PROTOCOL_CODEGEN_DIR}/tf_codegen.py
            ARGS  --spec     ${TF_SPEC_FILE}
                  --template ${JINJA_TEMPLATE}
                  --output   ${GENERATED_FILE}
            DEPENDS ${JINJA_GENERATE_TF_PROTOCOL_CODEGEN_DIR}/tf_codegen.py
                    ${JINJA_TEMPLATE}
        )
        list(APPEND GENERATED_FILES ${GENERATED_FILE})
    endforeach()
    add_custom_target(${JINJA_GENERATE_TARGET} ALL DEPENDS ${GENERATED_FILES} COMMENT "Generated files")

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()
