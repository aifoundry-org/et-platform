#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Compile the library for Master Minon.
add_library(device_trace_mm STATIC src/device_trace.c)
add_library(deviceFirmware::device_trace_mm ALIAS device_trace_mm)
target_compile_definitions(device_trace_mm PRIVATE -DMASTER_MINION=1)
target_include_directories(device_trace_mm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device_trace>
)
target_link_libraries(device_trace_mm
    PRIVATE
        deviceFirmware::device_common
        deviceFirmware::firmware_helpers_interface
)

set_target_properties(device_trace_mm PROPERTIES
    PUBLIC_HEADER "include/device_trace.h;include/device_trace_types.h"
)

install(TARGETS device_trace_mm
    EXPORT  EsperantoDeviceMinionRuntimeTargets
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device_trace
    COMPONENT device_trace
)

# Compile the library for Service processor and Computer Minon.
add_library(device_trace STATIC src/device_trace.c)
add_library(deviceFirmware::device_trace ALIAS device_trace)
target_compile_definitions(device_trace PRIVATE -DSP_OR_COMPUTE_MINION=1)
target_include_directories(device_trace
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device_trace>
)
target_link_libraries(device_trace
    PRIVATE
        deviceFirmware::device_common
        deviceFirmware::firmware_helpers_interface
)

set_target_properties(device_trace PROPERTIES
    PUBLIC_HEADER "include/device_trace.h;include/device_trace_types.h"
)

install(TARGETS device_trace
    EXPORT  EsperantoDeviceMinionRuntimeTargets
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device_trace
    COMPONENT device_trace
)
