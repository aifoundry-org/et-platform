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
project(logging VERSION 0.1.0)
find_package(g3log CONFIG REQUIRED)

add_library(logging INTERFACE)

add_library(logging::logging ALIAS logging)
target_compile_features(logging INTERFACE cxx_std_17)

include(GNUInstallDirs)

target_include_directories(logging
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  ${G3LOG_INCLUDE_DIRS}
)

target_link_libraries(logging INTERFACE ${G3LOG_LIBRARIES})


set_target_properties(logging PROPERTIES
  PUBLIC_HEADER "include/logging/Logging.h;include/logging/Logger.h;include/logging/DefaultSinks.h"
)


# Add package information
include(CMakePackageConfigHelpers)

# Add all targets to the build-tree export set
export(TARGETS logging
  FILE "${PROJECT_BINARY_DIR}/Logging.cmake"
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE logging)

install(TARGETS logging
  EXPORT LoggingTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common/logging
)

# Install the export set for use with the install-tree
install(EXPORT LoggingTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/logging
  NAMESPACE logging::
)


##### Package

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/LoggingConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/LoggingConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/logging
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/LoggingConfigVersion.cmake
  VERSION ${CMAKE_PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/LoggingConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/LoggingConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/logging
)