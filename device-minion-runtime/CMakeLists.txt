#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.5)
project(deviceMinionRuntime VERSION 0.0.1 LANGUAGES C ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

include(ExternalProject)
include(GNUInstallDirs)
include(DeviceUtils)
cmake_policy(SET CMP0076 NEW)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(ET_DEFAULT_BUILD_TYPE "Release")

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
option(ENABLE_REPORT_GLOBAL_FLAGS "Wether global flags should be shown when configuring the project" ON)
option(DEVICE_MINION_RUNTIME_DEPRECATED "Enable deprecated functionality" ON)
option(BUILD_DOC "Build documentation" ON)

set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")

if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "INFO")
    set(ENABLE_CMD_EXECUTION_TRACE ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "DEBUG")
else()
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "RELEASE")
endif()

set(DEVICE_MINION_RUNTIME_VERSION_MAJOR   0)
set(DEVICE_MINION_RUNTIME_VERSION_MINOR   0)
set(DEVICE_MINION_RUNTIME_REVISION_NUMBER 1)

if (DEVICE_MINION_RUNTIME_DEPRECATED)
    set(BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE STRING "Bin install dir")
    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING "Include install dir")
    set(LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE STRING "Library install dir")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(deviceApi REQUIRED)
find_package(signedImageFormat REQUIRED)
find_package(esperantoTrace REQUIRED)
find_package(etsoc_hal REQUIRED)

########## Documentation #####################################

if (BUILD_DOC)
    # check if Doxygen is installed
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
    add_subdirectory(doc)
    else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif()

############################ FW build Rules ############### ####################

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(tools)

### Add package information
include(CMakePackageConfigHelpers)


# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoDeviceMinionRuntime)
# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(EsperantoDeviceMinionRuntimeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfig.cmake
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoDeviceMinionRuntime/cmake
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)
# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EsperantoDeviceMinionRuntime
    COMPONENT machine-minion
)
# Install the export set for use with the install-tree
install(EXPORT EsperantoDeviceMinionRuntimeTargets
    NAMESPACE EsperantoDeviceMinionRuntime::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EsperantoDeviceMinionRuntime
)
