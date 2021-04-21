#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.17)
project(deviceLayer VERSION 0.1.0)

find_package(esperanto-device-api REQUIRED)
find_package(fmt REQUIRED)
find_package(linuxDriver REQUIRED)
find_package(sw-sysemu REQUIRED)
find_package(logging REQUIRED)
find_package(Threads REQUIRED)

get_target_property(PCIE_DRIVER_INC_DIR linuxDriver::etsoc1-pcie-driver INTERFACE_INCLUDE_DIRECTORIES)

include_directories(${PCIE_DRIVER_INC_DIR})

add_library(deviceLayer
  src/DeviceLayer.cpp
  src/DeviceSysEmu.cpp
  src/DevicePcie.cpp
  src/SysEmuHostListener.cpp)

add_library(deviceLayer::deviceLayer ALIAS deviceLayer)
target_compile_features(deviceLayer PUBLIC cxx_std_17)

include(GNUInstallDirs)

target_include_directories(deviceLayer
 PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/tests>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(deviceLayer PROPERTIES
  PUBLIC_HEADER "include/deviceLayer/IDeviceLayer.h;include/deviceLayer/mocks/IDeviceLayerMock.h;include/deviceLayer/mocks/IDeviceLayerFake.h"
  POSITION_INDEPENDENT_CODE TRUE
)
target_link_libraries(deviceLayer PRIVATE fmt::fmt stdc++fs Threads::Threads logging::logging)
target_link_libraries(deviceLayer PUBLIC sw-sysemu::sw-sysemu)
target_include_directories(deviceLayer PRIVATE ${ESPERANTO_DEVICE_API_INCLUDE_DIR})

# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS deviceLayer
  FILE "${PROJECT_BINARY_DIR}/DeviceLayer.cmake"
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE deviceLayer)

install(TARGETS deviceLayer
  EXPORT DeviceLayerTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-layer
)

# Install the export set for use with the install-tree
install(EXPORT DeviceLayerTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
  NAMESPACE deviceLayer::
)

add_subdirectory(tests)

##### Package

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceLayerConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfigVersion.cmake
  VERSION ${CMAKE_PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/DeviceLayerConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceLayer
)
