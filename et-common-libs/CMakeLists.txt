#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)

# Esperanto common libraries - used across SP, MM MMode, MM SMode, CM MMode,
# CM SMode, CM UMode.
project(
    et-common-libs
    VERSION 0.0.1
    LANGUAGES C
    DESCRIPTION "ET Accelerator API, a header and library exposed to CM usermode"
)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

include(GNUInstallDirs)

# Build and install Compute Minion User Mode libs
include(cm_umode.cmake)

# Build and install Minion Bootloader lib
include(minion_bl.cmake)

# Build and install Compute Minion Supervisory Mode libs
#include(cm_smode.cmake)

# Build and install Compute Minion Machine Mode libs
#include(cm_mmode.cmake)

# Build and install Master Minion Supervisory Mode libs
#include(mm_smode.cmake)

# Build and install Master Minion Machine Mode libs
#include(mm_mmode.cmake)

# Build and install Service Processor libs
#include(sp.cmake)


# Install 
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/et-common-libs-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/et-common-libs-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/et-common-libs
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/et-common-libs-config-version.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/et-common-libs-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/et-common-libs-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/et-common-libs
)
