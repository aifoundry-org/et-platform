#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

function(jinja_generate_apis_cpp SRCS HDRS)
    if(NOT ARGN)
        message(SEND_ERROR "Error: jinja_generate_apis_cpp() called without any jinja files")
        return()
    endif()

    set(options OPERATIONS MANAGEMENT)
    set(oneValueArgs TARGET DEVICE_API_CODEGEN_DIR)
    set(multiValueArgs JINJA_TEMPLATES)
    cmake_parse_arguments(JINJA_GENERATE_API "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT (JINJA_GENERATE_API_OPERATIONS OR JINJA_GENERATE_API_MANAGEMENT))
        message(SEND_ERROR "Error: jinja_generate_apis_cpp() called without argument: OPERATIONS | MANAGEMENT")
        return()
    endif()

    if (NOT JINJA_GENERATE_API_TARGET)
        message(SEND_ERROR "Error: jinja_generate_apis_cpp() called without argument: TARGET")
        return()
    endif()

    if (NOT JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR)
        message(SEND_ERROR "Error: jinja_generate_api_cpp() called without argument: DEVICE_API_CODEGEN_DIR")
        return()
    endif()

    if (NOT JINJA_GENERATE_API_JINJA_TEMPLATES)
        message(SEND_ERROR "Error: jinja_generate_apis_cpp() called without argument: JINJA_TEMPLATES")
        return()
    endif()

    file(GLOB_RECURSE DEVICE_API_SCHEMA_FILES RELATIVE ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR} *.yaml)

    if (JINJA_GENERATE_API_OPERATIONS)
        set(DEVICE_API_SPEC_FILE ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/operations-api/operations-api.yaml)
    elseif (JINJA_GENERATE_API_MANAGEMENT)
        set(DEVICE_API_SPEC_FILE ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/management-api/management-api.yaml)
    endif()

    if (JINJA_GENERATE_API_OPERATIONS)
        set(GENERATED_FILE_KEY "ops_api")
    elseif (JINJA_GENERATE_API_MANAGEMENT)
        set(GENERATED_FILE_KEY "mgmt_api")
    endif ()

    set(${SRCS})
    set(${HDRS})
    foreach(FIL ${JINJA_GENERATE_API_JINJA_TEMPLATES})
        get_filename_component(FIL_ABS  ${FIL} ABSOLUTE)  # file name absolute path
        get_filename_component(FIL_EXT  ${FIL} LAST_EXT)  # file name last extension (.c from d/a.b.c)
        get_filename_component(FIL_DIR  ${FIL} DIRECTORY) # Directory without file name
        get_filename_component(FIL_WE   ${FIL} NAME_WE)   # file name without directory or longest extension (a from d/a.b.c)
        get_filename_component(FIL_WLE  ${FIL} NAME_WLE)  # file name without directory or last extension (a.b from d/a.b.c)
        get_filename_component(SRC_EXT  ${FIL_WLE} LAST_EXT) # extract '.h' or '.cc/.cpp' from name

        if (NOT FIL_EXT STREQUAL ".jinja")
            message(SEND_ERROR "Error: jinja_generate_api_cpp() called with file without jinja extension (${FIL_EXT}). Abort")
            return()
        endif()

        # take a XXX.{h|cpp}.jinja file and generate a XXX_priviledged.{h|cpp} file
        # take a XXX.{h|cpp}.jinja file and generate a YYY/X_type_XX.{h|cpp} file
        set(JINJA_TEMPLATE      ${FIL_ABS})

        string(REPLACE "_" ";" FIL_LIST "${FIL_WE}${SRC_EXT}")
        list(REMOVE_AT FIL_LIST 1)
        list(INSERT    FIL_LIST 1 ${GENERATED_FILE_KEY})
        string(REPLACE ";" "_" GENERATED_FILE_NAME "${FIL_LIST}")

        set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis")
        if (JINJA_GENERATE_API_OPERATIONS)
            set(GENERATED_DIR  "${GENERATED_DIR}/${FIL_DIR}/operations-api")
        elseif (JINJA_GENERATE_API_MANAGEMENT)
            set(GENERATED_DIR  "${GENERATED_DIR}/${FIL_DIR}/management-api")
        endif()
        set(GENERATED_FILE "${GENERATED_DIR}/${GENERATED_FILE_NAME}")

        if (SRC_EXT MATCHES ".h")
            list(APPEND ${HDRS} ${GENERATED_FILE})
        else()
            list(APPEND ${SRCS} ${GENERATED_FILE})
        endif()

        add_custom_command(
            OUTPUT  ${GENERATED_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
            COMMAND ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/device_apis_codegen.py
            ARGS  --spec     ${DEVICE_API_SPEC_FILE}
                  --schema   ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/schema/device-apis.schema.json
                  --template ${JINJA_TEMPLATE}
                  --output   ${GENERATED_FILE}
            DEPENDS ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/device_apis_codegen.py
                    ${JINJA_GENERATE_API_DEVICE_API_CODEGEN_DIR}/schema/device-apis.schema.json
                    ${DEVICE_API_SCHEMA_FILES}
                    ${JINJA_TEMPLATE}
        )
        list(APPEND GENERATED_FILES ${GENERATED_FILE})
    endforeach()
    add_custom_target(${JINJA_GENERATE_API_TARGET} ALL DEPENDS ${GENERATED_FILES} COMMENT "Generated files")

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()
