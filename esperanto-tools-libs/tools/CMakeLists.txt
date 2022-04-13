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
find_package(EsperantoBootLoader REQUIRED)
find_package(EsperantoDeviceMinionRuntime REQUIRED)
get_property(BOOTROM_TRAMPOLINE_TO_BL2_ELF TARGET EsperantoBootLoader::BootromTrampolineToBL2.elf PROPERTY LOCATION)
get_property(BL2_ELF TARGET EsperantoBootLoader::ServiceProcessorBL2_fast-boot.elf PROPERTY LOCATION)
get_property(MASTER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MasterMinion.elf PROPERTY LOCATION)
get_property(MACHINE_MINION_ELF TARGET EsperantoDeviceMinionRuntime::MachineMinion.elf PROPERTY LOCATION)
get_property(WORKER_MINION_ELF TARGET EsperantoDeviceMinionRuntime::WorkerMinion.elf PROPERTY LOCATION)

add_library(benchmarker
  src/BenchmarkerImp.cpp
  src/BenchmarkerImp.h
  src/Logging.h
  src/Worker.h
  src/Worker.cpp
  include/tools/IBenchmarker.h)

add_library(runtimeTools::benchmarker ALIAS benchmarker)
target_compile_features(benchmarker PUBLIC cxx_std_17)

add_executable(runKernel src/runKernel.cpp)
target_include_directories(runKernel PRIVATE ../src)
target_compile_features(runKernel PRIVATE cxx_std_17)
target_link_libraries(runKernel PRIVATE hostUtils::logging runtime::etrt_static deviceLayer::deviceLayer)

find_package(gflags REQUIRED)
configure_file(src/Constants.h.in ${CMAKE_BINARY_DIR}/src/Constants.h)
add_executable(bench src/bench.cpp ${CMAKE_BINARY_DIR}/src/Constants.h)
target_include_directories(bench PRIVATE ${CMAKE_BINARY_DIR}/src)
target_link_libraries(bench PRIVATE gflags::gflags stdc++fs benchmarker)

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

target_link_libraries(benchmarker PRIVATE stdc++fs hostUtils::logging deviceLayer::deviceLayer)
target_link_libraries(benchmarker PUBLIC runtime::etrt_static)

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


