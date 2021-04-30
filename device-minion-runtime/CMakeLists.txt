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
project(DeviceFirmware VERSION 0.0.1 LANGUAGES C ASM)

set(DEVICE_MINION_RUNTIME_VERSION_MAJOR   ${CMAKE_MAJOR_VERSION})
set(DEVICE_MINION_RUNTIME_VERSION_MINOR   ${CMAKE_MINOR_VERSION})
set(DEVICE_MINION_RUNTIME_REVISION_NUMBER ${CMAKE_PATCH_VERSION})

include(ExternalProject)
include(GNUInstallDirs)
include(DeviceUtils)
cmake_policy(SET CMP0076 NEW)

set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")

find_package(esperanto-device-api CONFIG REQUIRED)
message("Esperanto Device API Found: ${esperanto-device-api_FOUND}")

set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

option(BUILD_DOC "Build documentation" ON)


#Make sure that custom modules are loaded
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

########## Documentation #####################################

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

####################### External package build dependencies ####################

add_subdirectory(external)

############################ FW build Rules ############### ####################

include_directories("${PROJECT_BINARY_DIR}/src/shared/include")

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
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
  )
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoDeviceMinionRuntimeConfigVersion.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoDeviceMinionRuntime
  COMPONENT machine-minion)

# Install the export set for use with the install-tree
install(EXPORT EsperantoDeviceMinionRuntimeTargets
  NAMESPACE EsperantoDeviceMinionRuntime::
  DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoDeviceMinionRuntime")
