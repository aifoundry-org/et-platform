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
project(deviceManagement VERSION 0.1.0)

find_package(deviceApi REQUIRED)

set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")

option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

add_library(deviceManagement
  src/DeviceManagement.cc
)

add_library(deviceManagement::deviceManagement ALIAS deviceManagement)
target_link_directories(deviceManagement PUBLIC ${CMAKE_INSTALL_PREFIX}/lib)
target_compile_features(deviceManagement PUBLIC cxx_std_17)

include(GNUInstallDirs)

target_include_directories(deviceManagement
 PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(deviceManagement PROPERTIES
  PUBLIC_HEADER include/deviceManagement/DeviceManagement.h
  POSITION_INDEPENDENT_CODE TRUE
)

target_link_libraries(deviceManagement PUBLIC deviceApi::deviceApi PRIVATE glog)

# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS deviceManagement
  FILE "${PROJECT_BINARY_DIR}/deviceManagement.cmake"
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE deviceManagement)


install(TARGETS deviceManagement
  EXPORT deviceManagementTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/device-management
)

# Generate shared library

add_library(DM SHARED $<TARGET_OBJECTS:deviceManagement>)

## Build shared library
install(TARGETS DM
  EXPORT deviceManagementTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

## Build shared library
install(TARGETS DM
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

# Install the export set for use with the install-tree
install(EXPORT deviceManagementTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
  NAMESPACE deviceManagement::
)

add_subdirectory(tests)
add_subdirectory(tools)

##### Package

# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS deviceManagement
  FILE "${PROJECT_BINARY_DIR}/deviceManagement.cmake"
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceManagementConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/deviceManagementConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deviceManagement
)
