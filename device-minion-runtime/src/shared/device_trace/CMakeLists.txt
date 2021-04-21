#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

add_library(device_trace STATIC
    device_trace.c
)
add_library(deviceFirmware::device_trace ALIAS device_trace)
target_compile_definitions(device_trace PUBLIC -DMASTER_MINION=1)

target_include_directories(device_trace
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/esperanto-fw/device_trace>
)

include_directories(${SHARED_INC_DIRS})

set_target_properties(device_trace PROPERTIES
    PUBLIC_HEADER "device_trace.h;device_trace_types.h"
)

install(TARGETS device_trace
    EXPORT  EsperantoDeviceMinionRuntimeTargets
    ARCHIVE       DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw
    PUBLIC_HEADER DESTINATION ${INCLUDE_INSTALL_DIR}/esperanto-fw/device_trace
    COMPONENT device_trace
)
