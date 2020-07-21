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

project(EsperantoRuntime VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/sanitizer-cmake/cmake")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoverageSupport.cmake)

option(BUILD_SHARED_LIBS "Build shared libs." ON)
option(ENABLE_PCIE "Enable pcie support by default" OFF)
option(ENABLE_CCACHE "Enable ccahe in the builds" OFF)
# If not empty install any runtime python packages
set(PYTHON "" CACHE PATH "Path to the python binary to install our python packages under")
set(DOCUMENTATION_INSTALL_DIR "${CMAKE_INSTALL_DIR}/doc" CACHE PATH "Documentation installation path")

execute_process(
  COMMAND git rev-parse --verify HEAD
  OUTPUT_VARIABLE ET_RUNTIME_GIT_HASH
  TIMEOUT 20
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

if(ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    set(EXTERNAL_PROJECT_CCACHE -DCMAKE_CXX_COMPILER_LAUNCHER:STRING=${CCACHE_PROGRAM} -DCMAKE_C_COMPILER_LAUNCHER:STRING=${CCACHE_PROGRAM})
    set(EXTERNAL_PROJECT_ENABLE_CCACHE -DENABLE_CCACHE:BOOL=ON)
  endif(CCACHE_PROGRAM)
endif(ENABLE_CCACHE)

set(DEPENDENCIES_PATH "/usr" CACHE PATH "Path where runtime dependencies are installed")
set(SYSEMU_PATH  ${DEPENDENCIES_PATH}/bin/sys_emu)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  ${CMAKE_INSTALL_PREFIX}/lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc/et-runtime/ CACHE STRING "Config install dir")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

# System dependencies
find_package(Sanitizers REQUIRED)
find_package(gflags REQUIRED)
find_package(Glog REQUIRED)
find_package(fmt REQUIRED)
find_package(GTest REQUIRED)
find_package(elfio REQUIRED)
find_package(absl REQUIRED)

# Esperanto dependencies
find_package(SimulatorAPI REQUIRED)
find_package(esperanto-device-api REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(EsperantoTestKernels REQUIRED)

set(MINION_RUNTIME_PACKAGE_NAME EsperantoDeviceMinionRuntime)
message("EsperantoDeviceMinionRuntime found: " ${EsperantoDeviceMinionRuntime_FOUND})
set(MINION_RUNTIME_FOUND 1)
set(ESPERANTO_FW_INCLUDE_DIR ${ESPERANTO_DEVICE_MINION_RUNTIME_INCLUDE_DIR})

# Add rules for auto-generated headers.
add_subdirectory(include)
add_subdirectory(docs)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)

### Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS etrt etrt_static
  FILE "${PROJECT_BINARY_DIR}/EsperantoRuntimeTargets.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoRuntime)

# Create the EsperantoRuntimeConfig.cmake and EsperantoRuntimeConfigVersion files
configure_package_config_file(EsperantoRuntimeConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoRuntimeConfig.cmake
  INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoRuntime/cmake
  PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoRuntimeConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the EsperantoRuntimeConfig.cmake and EsperantoRuntimeConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoRuntimeConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoRuntimeConfigVersion.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoRuntime
  COMPONENT runtime)

# Install the export set for use with the install-tree
install(EXPORT EsperantoRuntimeTargets DESTINATION
  "${LIB_INSTALL_DIR}/cmake/EsperantoRuntime" COMPONENT runtime)
