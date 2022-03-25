#------------------------------------------------------------------------------
# Copyright (C) 2022, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(GoogleTest)
find_package(EsperantoTestKernels REQUIRED)
find_package(firmwareTests REQUIRED)

set(KERNELS_DIR "${ESPERANTO_TEST_KERNELS_LIB_DIR}/esperanto-fw/kernels")

enable_testing()

configure_file(run_gdb.sh.in ${CMAKE_CURRENT_BINARY_DIR}/run_gdb.sh @ONLY)
configure_file(break_point.gdb.in ${CMAKE_CURRENT_BINARY_DIR}/break_point.gdb @ONLY)
file(COPY mem_read.gdb DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY reg_read_write.gdb DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY reconnect.gdb DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

function(create_gdb_test)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_test(
        NAME			${TEST_NAME}
        WORKING_DIRECTORY	${CMAKE_CURRENT_BINARY_DIR}
        COMMAND			run_gdb.sh ${TEST_ARGS}
    )

    if(NOT "${TEST_PROPERTIES}" STREQUAL "")
        set_tests_properties(${TEST_NAME} PROPERTIES ${TEST_PROPERTIES})
    endif()
endfunction()

create_gdb_test(
    NAME	DebugServer:RunGDB.breakPointTest
    ARGS	--gdb_script ${CMAKE_CURRENT_BINARY_DIR}/break_point.gdb
    PROPERTIES	"LABELS;GDBSERVER;TIMEOUT;300"
)

create_gdb_test(
    NAME	DebugServer:RunGDB.registerReadWriteTest
    ARGS	--gdb_script ${CMAKE_CURRENT_BINARY_DIR}/reg_read_write.gdb
    PROPERTIES	"LABELS;GDBSERVER;TIMEOUT;120"
)

create_gdb_test(
    NAME	DebugServer:RunGDB.memoryReadTest
    ARGS	--gdb_script ${CMAKE_CURRENT_BINARY_DIR}/mem_read.gdb
    PROPERTIES	"LABELS;GDBSERVER;TIMEOUT;120"
)

create_gdb_test(
    NAME	DebugServer:RunGDB.reconnectTest
    ARGS	--gdb_script ${CMAKE_CURRENT_BINARY_DIR}/reconnect.gdb
    PROPERTIES	"LABELS;GDBSERVER;TIMEOUT;120"
)
