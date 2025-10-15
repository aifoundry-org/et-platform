# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

if (NOT DEFINED GIT_HASH_STRING)
  execute_process(COMMAND git rev-parse HEAD
    OUTPUT_VARIABLE GIT_HASH_STRING
    RESULT_VARIABLE RES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 20
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (RES)
    message(FATAL_ERROR "Get git hash string failed with: ${OUTPUT_VARIABLE}")
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/device_apis_build_configuration.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis/device_apis_build_configuration.h)

# header-only library for facilitating propagation of includes
add_library(deviceApi INTERFACE)
target_include_directories(deviceApi
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/esperanto>
)
# Alias that should be consumed by all users
add_library(deviceApi::deviceApi ALIAS deviceApi)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/esperanto/device-apis/device_apis_build_configuration.h
          ${CMAKE_CURRENT_SOURCE_DIR}/device_apis_message_types.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis
    COMPONENT deviceApi)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/management-api
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis/
    COMPONENT deviceApi)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/operations-api
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto/device-apis/
    COMPONENT deviceApi)
