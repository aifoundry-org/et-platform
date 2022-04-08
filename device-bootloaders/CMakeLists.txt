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
project(EsperantoBootLoader VERSION 0.1.0 LANGUAGES C ASM)

message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

if (DEFINED GIT_HASH_STRING)
    set(GIT_HASH_STRING "\"${GIT_HASH_STRING}\"")
endif()
if (DEFINED GIT_VERSION_STRING)
    set(GIT_VERSION_STRING "\"${GIT_VERSION_STRING}\"")
endif()
message(STATUS "GIT_HASH_STRING:    ${GIT_HASH_STRING}")
message(STATUS "GIT_HASH_ARRAY:     ${GIT_HASH_ARRAY}")
message(STATUS "GIT_VERSION_STRING: ${GIT_VERSION_STRING}")
message(STATUS "GIT_VERSION_ARRAY:  ${GIT_VERSION_ARRAY}")

set(ET_DEFAULT_BUILD_TYPE "Release")

########## Project options ###################################

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
option(ENABLE_REPORT_GLOBAL_FLAGS "Wether global flags should be shown when configuring the project" ON)
include(CommonProjectSettings)
include(DeviceUtils)
include(GNUInstallDirs)

cmake_policy(SET CMP0076 NEW)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
if(ENABLE_WARNINGS_AS_ERRORS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

option(BUILD_DOC "Build documentation" ON)

########## Dependencies #####################################

find_package(signedImageFormat REQUIRED)
find_package(deviceApi REQUIRED)
find_package(EsperantoFlashTool CONFIG REQUIRED)
find_package(tf-protocol REQUIRED)
find_package(etsoc_hal REQUIRED)
find_package(esperantoTrace REQUIRED)
find_package(et-common-libs REQUIRED COMPONENTS sp-bl1 sp-bl2)

# installation related vars (to replace in the future for GnuInstallDirs)
set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

########## Documentation #####################################

if (BUILD_DOC)
    # check if Doxygen is installed
    find_package(Doxygen REQUIRED)
    
    add_subdirectory(doc)
endif()

############################ FW build Rules ############### 

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(scripts)

########## Installation #####################################

include(CMakePackageConfigHelpers)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoBootLoaderTargets)

# Install the export set for use with the install-tree
install(EXPORT EsperantoBootLoaderTargets
    NAMESPACE EsperantoBootLoader::
    DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoBootLoader"
)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/EsperantoBootLoaderConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfig.cmake
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoBootLoader/cmake
    PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoBootLoader
    COMPONENT bootloader
)

