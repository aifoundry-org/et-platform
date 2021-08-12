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
project(EsperantoBootLoader VERSION 0.1.0 LANGUAGES C ASM)

set(ET_DEFAULT_BUILD_TYPE "Release")

option(ENABLE_STRICT_BUILD_TYPES "Whether project configuration should fail if build_type is not one of the allowed values: Debug Release RelWithDebInfo MinSizeRel" ON)
option(ENABLE_REPORT_GLOBAL_FLAGS "Wether global flags should be shown when configuring the project" ON)
include(CommonProjectSettings)
include(DeviceUtils)
include(GNUInstallDirs)
cmake_policy(SET CMP0076 NEW)

find_package(EsperantoDeviceMinionRuntime REQUIRED)
find_package(signedImageFormat REQUIRED)
find_package(deviceApi REQUIRED)
find_package(tf-protocol REQUIRED)
find_package(EsperantoFlashTool CONFIG REQUIRED)
find_package(etsoc_hal REQUIRED)

# Providers for third-party dependencies (gRPC_*_PROVIDER properties):
# "module": build the dependency using sources from git submodule under external
# "package": use cmake's find_package functionality to locate a pre-installed dependency
# TODO enable correct use of package to find preinstalled dependencies for now "package"
# will mean to use the DEPENDENCIES_PATH to derive the include and library paths
# FIXME currenlty this project is not self contained and does not contain any external dependencies.
# set(devicefw_CODE_SIGNING_TOOLS_PROVIDER "module" CACHE STRING "Provider of code signing tools library")
# set_property(CACHE devicefw_CODE_SIGNING_TOOLS_PROVIDER PROPERTY STRINGS "module" "package")
# set(devicefw_FLASH_TOOL_PROVIDER "module" CACHE STRING "Provider of the flash tools library")
# set_property(CACHE devicefw_FLASH_TOOL_PROVIDER PROPERTY STRINGS "module" "package")
set(DOCUMENTATION_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}" CACHE STRING "Documentation installation path")
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")

set(DEVFW_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

list(APPEND CMAKE_PREFIX_PATH ${DEVFW_EXTERNAL_INSTALL_PREFIX})

option(BUILD_DOC "Build documentation" ON)

# Make sure that custom modules are loaded
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

########## Documentation #####################################

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

############################ FW build Rules ############### ####################

include_directories(${PROJECT_SOURCE_DIR}/shared/include)

get_filename_component(SHARED_SRC_DIR "shared/src" ABSOLUTE)
set(SHARED_INC_DIRS ${PROJECT_SOURCE_DIR}/shared/include)

get_target_property(FLASH_TOOL_HEADERS_INTERFACE_PATH flash-tool INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${FLASH_TOOL_HEADERS_INTERFACE_PATH})



add_subdirectory(external)
add_subdirectory(src)


include(CMakePackageConfigHelpers)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE EsperantoBootLoaderTargets)

# Create the *-config.cmake and *-config-version.cmake files
configure_package_config_file(EsperantoBootLoaderConfig.cmake.in
     ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfig.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/EsperantoBootLoader/cmake
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR LIB_INSTALL_DIR BIN_INSTALL_DIR
  )
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion )

# Install the *Config.cmake and *ConfigVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EsperantoBootLoaderConfigVersion.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/EsperantoBootLoader
  COMPONENT bootloader)

# Install the export set for use with the install-tree
install(EXPORT EsperantoBootLoaderTargets
  NAMESPACE EsperantoBootLoader::
  DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoBootLoader")
