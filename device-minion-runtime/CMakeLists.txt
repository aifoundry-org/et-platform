#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(FIRMWARE_TOOLS_DIR "firmware-tools" ABSOLUTE)

cmake_minimum_required(VERSION 3.5)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(DeviceFirmware VERSION 0.1.0 LANGUAGES C ASM)

set(CMAKE_VERBOSE_MAKEFILE OFF)
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

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}/include")

add_subdirectory(src)
add_subdirectory(tools)


### Add package information
include(CMakePackageConfigHelpers)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE esperanto-fw)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(esperanto-fw-config.cmake.in
     ${CMAKE_CURRENT_BINARY_DIR}/esperanto-fw-config.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/cmake
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
  )
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/esperanto-fw-config-version.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/esperanto-fw-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/esperanto-fw-config-version.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/esperanto-fw
  COMPONENT machine-minion)

# Install the export set for use with the install-tree
install(EXPORT esperanto-fw-targets
  NAMESPACE esperanto-fw::
  DESTINATION "${LIB_INSTALL_DIR}/cmake/esperanto-fw")
