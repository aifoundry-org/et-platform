#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.13)
project(deviceMinionRuntime VERSION 0.7.0 LANGUAGES C ASM)

message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")
set(DEVICE_MINION_RUNTIME_VERSION_MAJOR   ${PROJECT_VERSION_MAJOR})
set(DEVICE_MINION_RUNTIME_VERSION_MINOR   ${PROJECT_VERSION_MINOR})
set(DEVICE_MINION_RUNTIME_REVISION_NUMBER ${PROJECT_VERSION_PATCH})

message(STATUS "GIT_HASH_STRING:    ${GIT_HASH_STRING}")
message(STATUS "GIT_HASH_ARRAY:     ${GIT_HASH_ARRAY}")
message(STATUS "GIT_VERSION_STRING: ${GIT_VERSION_STRING}")
message(STATUS "GIT_VERSION_ARRAY:  ${GIT_VERSION_ARRAY}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

include(ExternalProject)
include(GNUInstallDirs)
include(DeviceUtils)
cmake_policy(SET CMP0076 NEW)

set(ET_DEFAULT_BUILD_TYPE "Release")

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
option(ENABLE_REPORT_GLOBAL_FLAGS "Wether global flags should be shown when configuring the project" ON)
option(DEVICE_MINION_RUNTIME_DEPRECATED "Enable deprecated functionality" ON)
option(BUILD_DOC "Build documentation" ON)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
include(CompilerWarnings)

set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")

if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "INFO")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "DEBUG")
else()
    set(DEVICE_MINION_RUNTIME_BUILD_TYPE "RELEASE")
endif()

if (DEVICE_MINION_RUNTIME_DEPRECATED)
    set(BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE STRING "Bin install dir")
    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING "Include install dir")
    set(LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE STRING "Library install dir")
endif()


find_package(deviceApi REQUIRED)
find_package(signedImageFormat REQUIRED)
find_package(esperantoTrace REQUIRED)
find_package(etsoc_hal REQUIRED)
find_package(et-common-libs REQUIRED COMPONENTS minion-bl mm-rt-svcs cm-rt-svcs)

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

add_subdirectory(src)
add_subdirectory(scripts)

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
