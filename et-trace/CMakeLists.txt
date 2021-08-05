#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.14)
project(deviceTrace LANGUAGES C)

option(DTRACE_TEST "Build device_trace tests" OFF)

add_library(device_trace_headers INTERFACE)
add_library(deviceTrace::headers ALIAS device_trace_headers)
target_include_directories(device_trace_headers
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device_trace>
)

if (DTRACE_TEST)
  message(STATUS "Building device_trace tests")
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS device_trace_headers
    EXPORT deviceTraceTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT deviceTraceTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceTrace
    NAMESPACE deviceTrace::
)
