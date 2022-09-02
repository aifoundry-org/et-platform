#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.15)
project(EsperantoTestKernels VERSION 1.4.0 LANGUAGES C ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(ExternalProject)
include(GNUInstallDirs)
include(DeviceUtils)
include(EsperantoTestKernelsUtils)

cmake_policy(SET CMP0076 NEW)

# Dependencies
find_package(esperantoTrace REQUIRED)
find_package(et-common-libs REQUIRED COMPONENTS cm-umode)


set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

### project contents (sources, tools, tests, etc)

add_subdirectory(src)


### Installation

include(CMakePackageConfigHelpers)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoTestKernelsTargets)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(EsperantoTestKernelsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfig.cmake
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoTestKernels/cmake
    PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoTestKernelsConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoTestKernels
    COMPONENT machine-minion
)

# Install the export set for use with the install-tree
install(EXPORT EsperantoTestKernelsTargets
    NAMESPACE esperanto-test-kernels::
    DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoTestKernels
)
