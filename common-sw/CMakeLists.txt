#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.15)
project(hostUtils VERSION 0.1.0 LANGUAGES CXX)


list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

option(ENABLE_STRICT_BUILD_TYPES "Project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
set(ET_DEFAULT_BUILD_TYPE Release)
include(CommonProjectSettings)

option(ENABLE_WARNINGS_AS_ERRORS "Targets should treat warnings as errors" OFF)
include(CompilerWarnings)


add_subdirectory(logging)
add_subdirectory(threadPool)
add_subdirectory(debug)


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