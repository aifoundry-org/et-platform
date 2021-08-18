#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(esperantoTrace
  VERSION 0.1.0
  DESCRIPTION "Esperanto device traces"
  LANGUAGES C)

option(ET_TRACE_TEST "Build et-trace tests" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

#---------------
# Trace library
#---------------

add_library(et_trace INTERFACE)
add_library(esperantoTrace::et_trace ALIAS et_trace)
target_include_directories(et_trace
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
)

if (ET_TRACE_TEST)
  message(STATUS "Building et-trace tests")
  enable_testing()
  add_subdirectory(tests)
endif()

#-------------------
# Configure package 
#-------------------

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/esperantoTraceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/esperantoTraceConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esperantoTrace
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/esperantoTraceConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

#-----------------
# Install targets
#-----------------

install(TARGETS et_trace
  EXPORT esperantoTraceTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto COMPONENT et_trace
)

install(EXPORT esperantoTraceTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esperantoTrace
  NAMESPACE esperantoTrace::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/esperantoTraceConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/esperantoTraceConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esperantoTrace
)
