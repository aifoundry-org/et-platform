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

include(ExternalProject)
include(GNUInstallDirs)
cmake_policy(SET CMP0076 NEW)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(DeviceFirmware VERSION 0.1.0 LANGUAGES C ASM)

# Providers for third-party dependencies (gRPC_*_PROVIDER properties):
# "module": build the dependency using sources from git submodule under external
# "package": use cmake's find_package functionality to locate a pre-installed dependency
# TODO enable correct use of package to find preinstalled dependencies for now "package"
# will mean to use the DEPENDENCIES_PATH to derive the include and library paths
set(devicefw_DEVICE_API_PROVIDER "module" CACHE STRING "Provider of device-api library")
set_property(CACHE devicefw_DEVICE_API_PROVIDER PROPERTY STRINGS "module" "package")
set(devicefw_CODE_SIGNING_TOOLS_PROVIDER "module" CACHE STRING "Provider of code signing tools library")
set_property(CACHE devicefw_CODE_SIGNING_TOOLS_PROVIDER PROPERTY STRINGS "module" "package")
set(devicefw_FLASH_TOOL_PROVIDER "module" CACHE STRING "Provider of the flash tools library")
set_property(CACHE devicefw_FLASH_TOOL_PROVIDER PROPERTY STRINGS "module" "package")

option(ENABLE_ZEBU_BAUD_RATE "Set the baud rate to be that fo the current Zebu builds" OFF)
# Temporary option to enable building the SP ROM as part of device-firmware to be used
# to transition/move the SP_ROM in sw-platform
option(BUILD_SP_ROM "Enable building the SP-ROM in this module" ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

set(EXTERNAL_PROJECT_DEPENDENCIES "" CACHE STRING "External project list")

set(DEVFW_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

list(APPEND CMAKE_PREFIX_PATH ${DEVFW_EXTERNAL_INSTALL_PREFIX})

option(BUILD_DOC "Build documentation" ON)
option(BUILD_FLASH_IMAGE "Build the flash image" OFF)

# Git configuration options to pass when building projects we clone
# when building an external project
set(GIT_CONFIG_OPTIONS "http.sslVerify=false http.proxy=http://sc-squid-sw.esperanto.ai:3128")

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

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}/include")

# List of files the flash image depends on: i.e. *.elf files etc
set(FLASH_IMAGE_DEPS "" CACHE INTERNAL "")

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
