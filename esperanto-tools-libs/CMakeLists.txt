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

option(HOST_SW "Enable building for the host-sw repo")
option(BUILD_SHARED_LIBS "Build shared libs." ON)
option(ETSOC "Build for the ETSOC")
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PCIE "Enable pcie support by default" OFF)
option(ENABLE_CCACHE "Enable ccahe in the builds" OFF)
## FIXME this option needs to be deprecated and etsoc will need to start
## setting the --sysemu runtime option eventually
option(ENABLE_SYSEMU_ARTIFACT "Enable artifactory path for etsoc builds" OFF)
option(BUILD_DEPENDENCIES "Build all dependencies from source" OFF)

# Providers for third-party dependencies (gRPC_*_PROVIDER properties):
# "module": build the dependency using sources from git submodule under external
# "package": use cmake's find_package functionality to locate a pre-installed dependency
# TODO enable correct use of package to find preinstalled dependencies for now "package"
# will mean to use the DEPENDENCIES_PATH to derive the include and library paths
set(runtime_DEVICE_API_PROVIDER "module" CACHE STRING "Provider of device-api library")
set_property(CACHE runtime_DEVICE_API_PROVIDER PROPERTY STRINGS "module" "package")
set(runtime_SIMULATOR_API_PROVIDER "module" CACHE STRING "Provider of simualtor-api library")
set_property(CACHE runtime_DEVICE_API_PROVIDER PROPERTY STRINGS "module" "package")
set(runtime_SW_SYSEMU_PROVIDER "module" CACHE STRING "Provider of the sw-sysemu executable")
set_property(CACHE runtime_SW_SYSEMU_PROVIDER PROPERTY STRINGS "module" "package")
set(runtime_ELFIO_PROVIDER "module" CACHE STRING "Provider of the sw-sysemu executable")
set_property(CACHE runtime_ELFIO_PROVIDER PROPERTY STRINGS "module" "package")

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

# To be overwritten in cases where we want to also "release" the external tools
# used by the runtime
set(RT_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

if(NOT "${CMAKE_INSTALL_PREFIX}" STREQUAL "")
  set(RT_EXTERNAL_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH
     "Instalation path of external dependencies" FORCE)
endif()

message(STATUS "CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "HOST_SW=" ${HOST_SW})
message(STATUS "ETSOC=" ${ETSOC})
message(STATUS "ENABLE_TESTS=" ${ENABLE_TESTS})

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()

set(external_project_dependencies "" CACHE STRING "External project list")

set(EXTERNAL_INCLUDE_DIRECTORIES
  ${RT_EXTERNAL_INSTALL_PREFIX}/include
  ${DEPENDENCIES_PATH}/include
  )

set(EXTERNAL_LINK_DIRECTORIES
  ${RT_EXTERNAL_INSTALL_PREFIX}/lib64
  ${RT_EXTERNAL_INSTALL_PREFIX}/lib
  ${DEPENDENCIES_PATH}/lib64
  ${DEPENDENCIES_PATH}/lib
  )


# List of packages to find

if (NOT BUILD_DEPENDENCIES)
  find_package(gflags)
endif()

if(NOT BUILD_DEPENDENCIES)
  find_package(Glog)
endif()

if(NOT BUILD_DEPENDENCIES)
  find_package(fmt CONFIG)
endif()

if(NOT BUILD_DEPENDENCIES)
  find_package(GTest)
endif()

if(NOT BUILD_DEPENDENCIES)
  find_package(elfio)
endif()
message("ELFIO FOUND ${elfio_FOUND}")

# Build any external dependencies
add_subdirectory(external)
list(APPEND CMAKE_PREFIX_PATH ${RT_EXTERNAL_INSTALL_PREFIX})

find_package(absl CONFIG REQUIRED)
message("ABSEIL Found : '${absl_FOUND}'")

find_package(SimulatorAPI CONFIG REQUIRED)
message("Sim API Found: ${SimulatorAPI_FOUND}")

find_package(esperanto-device-api CONFIG REQUIRED
  PATHS "${RT_EXTERNAL_INSTALL_PREFIX}/lib/cmake/esperanto-device-api"
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_DEFAULT_PATH
)
message("Espernato Device API Found: ${espernato-device-api_FOUND}")

# Currenly esperanto-fw is not required. Will be the case once we get completly rid
# of the fake-fw. Also prevert CMake from using the default search path currently it
# needs to be specified manually
find_package(esperanto-fw
  NO_DEFAULT_PATH
  )
message("Espernato-FW found: " ${esperanto-fw_FOUND})

# Add rules for auto-generated headers.
add_subdirectory(include)
add_subdirectory(docs)
add_subdirectory(src)
if (ENABLE_TESTS)
  add_subdirectory(tests)
  add_subdirectory(tools)
endif(ENABLE_TESTS)


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
