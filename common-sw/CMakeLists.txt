# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.21)
project(hostUtils VERSION 0.5.0 LANGUAGES CXX)


list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(ENABLE_STRICT_BUILD_TYPES "Project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
set(ET_DEFAULT_BUILD_TYPE Release)
include(CommonProjectSettings)

option(ENABLE_WARNINGS_AS_ERRORS "Targets should treat warnings as errors" OFF)
include(CompilerWarnings)

if (BUILD_TESTS)
  enable_testing()
endif()

add_subdirectory(src)

##### Package

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/hostUtilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/hostUtilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/hostUtilsConfig-version.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hostUtilsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hostUtilsConfig-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hostUtils
)
