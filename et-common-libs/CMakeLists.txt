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
project(et-common-libs VERSION 0.7.0 LANGUAGES C DESCRIPTION "ET Accelerator API, a header and library exposed to CM usermode")


option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# Find the required packages
find_package(esperantoTrace REQUIRED)

include(GNUInstallDirs)

option(WITH_SP_BL "Build sp_bl1/sp_bl2 component" ON)
if (WITH_SP_BL)
    # Build and install SP Bootloader lib
    include(sp_bl1.cmake)
    include(sp_bl2.cmake)
endif()

option(WITH_CM_UMODE "Build cm_umode component" ON)
if (WITH_CM_UMODE)
    # Build and install Compute Minion User Mode libs
    include(cm_umode.cmake)
endif()

option(WITH_MINION_BL "Build minion_bl component" ON)
if (WITH_MINION_BL)
    # Build and install Minion Bootloader lib for m-mode
    include(minion_bl.cmake)
endif()

option(WITH_MM_RT_SVCS "Build mm_rt_svcs component" ON)
if (WITH_MM_RT_SVCS)
    # Build and install Master Minion Run Time Services lib
    include(mm_rt_svcs.cmake)
endif()

option(WITH_CM_RT_SVCS "Build cm_rt_svcs component" ON)
if (WITH_CM_RT_SVCS)
    # Build and install Compute Minion Run Time Services lib
    include(cm_rt_svcs.cmake)
endif()

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
