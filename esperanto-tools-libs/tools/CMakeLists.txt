#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.17)
project(runtimeTools VERSION 0.1.0)

find_package(deviceLayer REQUIRED)
find_package(EsperantoTestKernels REQUIRED)

add_library(benchmarker
  src/BenchmarkerImp.cpp
  src/BenchmarkerImp.h
  include/tools/IBenchmarker.h)

add_library(runtimeTools::benchmarker ALIAS benchmarker)
target_compile_features(benchmarker PUBLIC cxx_std_17)

include(GNUInstallDirs)

target_include_directories(benchmarker
 PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(benchmarker PROPERTIES
  PUBLIC_HEADER "include/tools/IBenchmarker.h"
  POSITION_INDEPENDENT_CODE TRUE
)

target_link_libraries(benchmarker PRIVATE stdc++fs hostUtils::logging runtime::etrt_static deviceLayer::deviceLayer)

# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS benchmarker
  FILE "${PROJECT_BINARY_DIR}/benchmarker.cmake"
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE benchmarker)

install(TARGETS benchmarker
  EXPORT BenchmarkerTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tools
)

# Install the export set for use with the install-tree
install(EXPORT BenchmarkerTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tools
  NAMESPACE runtimeTools::
)

##### Package

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ToolsConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tools
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfigVersion.cmake
  VERSION ${CMAKE_PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tools
)
