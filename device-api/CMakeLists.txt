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

project(EsperantoDeviceAPI
  VERSION 0.1.0
  DESCRIPTION "Esperanto DeviceAPI Project"
  LANGUAGES C CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/CMake)

#set(CMAKE_VERBOSE_MAKEFILE ON)
option(ENABLE_DEVICEFW_TRACE "Enable device Tracing" ON)
option(ENABLE_CCACHE "Enable ccahe in the builds" OFF)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  ${CMAKE_INSTALL_PREFIX}/lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc/et-runtime/ CACHE STRING "Config install dir")

if(ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    set(EXTERNAL_PROJECT_CCACHE -DCMAKE_CXX_COMPILER_LAUNCHER=${CCACHE_PROGRAM} -DCMAKE_C_COMPILER_LAUNCHER=${CCACHE_PROGRAM})
    set(EXTERNAL_PROJECT_ENABLE_CCACHE -DENABLE_CCACHE=ON)
  endif(CCACHE_PROGRAM)
endif(ENABLE_CCACHE)

enable_testing()


############################ Documentation #####################################

option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

add_subdirectory(src)

### Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
#export(TARGETS gen-device-api
#  FILE "${PROJECT_BINARY_DIR}/esperanto-device-api-targets.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE esperanto-device-api)

# Create the esperanto-device-api-config.cmake and esperanto-device-api-vonfig-version files
configure_package_config_file(esperanto-device-api-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config.cmake
  INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/esperanto-device-api/cmake
  PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config-version.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the esperanto-device-apiConfig.cmake and esperanto-device-api-config-version.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/esperanto-device-api-config-version.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/esperanto-device-api
  COMPONENT device-api)

# Install the export set for use with the install-tree
#install(EXPORT esperanto-device-api-targets DESTINATION
#  "${LIB_INSTALL_DIR}/cmake/esperanto-device-api" COMPONENT device-api)
