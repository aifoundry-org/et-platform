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

# First step as the artifacts of this package are consumed by the toolchain script

# FIXME this a required package once the esperanto-fw package has been eliminated
find_package(EsperantoDeviceMinionRuntime REQUIRED)

set(MINION_RUNTIME_PACKAGE_NAME EsperantoDeviceMinionRuntime)

# Must set CMAKE_TOOLCHAIN_FILE before project() is called
set(CMAKE_TOOLCHAIN_FILE Toolchain-gcc-riscv.cmake)

project(EsperantoBootLoader VERSION 0.1.0 LANGUAGES C ASM)

### Add package information
include(CMakePackageConfigHelpers)

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

set(BOOTLOADERS_EXPORT_NAME EsperantoBootLoaderTargets)
set(BIN_INSTALL_DIR bin CACHE STRING "Bin install dir")
set(INCLUDE_INSTALL_DIR include/ CACHE STRING "Include install dir")
set(LIB_INSTALL_DIR  lib/  CACHE STRING "Library install dir")
set(SYSCONFIG_INSTALL_DIR etc/et-fw/ CACHE STRING "Config install dir")
set(ROM_TOOLS_DIR ${CMAKE_INSTALL_PREFIX}/bin/esperanto-fw/ServiceProcessorROM)

set(DEVFW_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

list(APPEND CMAKE_PREFIX_PATH ${DEVFW_EXTERNAL_INSTALL_PREFIX})

option(BUILD_DOC "Build documentation" ON)

############################ FW build Rules ############### ####################

# Download the ETSOC_HAL generated files
add_subdirectory(shared/include)
include_directories("${PROJECT_SOURCE_DIR}/shared/include/")
include_directories("${PROJECT_SOURCE_DIR}/shared/include/etsoc_hal")
include_directories("${PROJECT_SOURCE_DIR}/shared/include/etsoc_hal/inc")


# List of files the flash image depends on: i.e. *.elf files etc
set(FLASH_IMAGE_DEPS "" CACHE INTERNAL "")

get_filename_component(SHARED_SRC_DIR "shared/src" ABSOLUTE)
set(SHARED_INC_DIRS "${PROJECT_SOURCE_DIR}/shared/include")

  #FreeRTOS
set(FREERTOS_DIR "shared/FreeRTOS")

#FreeRTOS source
file(GLOB FREERTOS_SRC_FILES "${FREERTOS_DIR}/Source/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.S")
                             #"${FREERTOS_DIR}/Source/mem_mang/heap_3.c" not using FreeRTOS heap. Heap is only for libc.

#FreeRTOS includes
get_filename_component(FREERTOS_INC_DIR "${FREERTOS_DIR}/Source/include/" ABSOLUTE)
get_filename_component(FREERTOS_PORT_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/" ABSOLUTE)
get_filename_component(FREERTOS_CHIP_SPECIFIC_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/chip_specific_extensions/etsoc_RV64IMF" ABSOLUTE)
set(FREERTOS_INC_DIRS ${FREERTOS_INC_DIR} ${FREERTOS_PORT_INC_DIR} ${FREERTOS_CHIP_SPECIFIC_INC_DIR})

find_package(EsperantoFlashTool CONFIG REQUIRED)
get_target_property(FLASH_TOOL_HEADERS_INTERFACE_PATH flash-tool INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${FLASH_TOOL_HEADERS_INTERFACE_PATH})

find_package(EsperantoCryptoAPI CONFIG REQUIRED)

message("Crypto API Found: ${EsperantoCryptoAPI_FOUND}")
get_target_property(CRYPTO_HEADERS_INTERFACE_PATH crypto_api INTERFACE_INCLUDE_DIRECTORIES)
set(CRYPTO_HEADERS ${CRYPTO_HEADERS_INTERFACE_PATH})

get_property(FW_TYPES_HELPERS_INCLUDE_PATH
  TARGET  ${MINION_RUNTIME_PACKAGE_NAME}::fw_helpers
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${FW_TYPES_HELPERS_INCLUDE_PATH})

find_package(EsperantoServiceProcessorROM REQUIRED)

get_property(VAULTIP_HELPERS_INCLUDE_PATH
  TARGET EsperantoServiceProcessorROM::vaultip
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${VAULTIP_HELPERS_INCLUDE_PATH})

get_property(SP_ROM_OTP_TYPES_PATH
  TARGET  EsperantoServiceProcessorROM::sp_rom_otp_types
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${SP_ROM_OTP_TYPES_PATH})

add_subdirectory(BootromTrampolineToBL2)
add_subdirectory(ServiceProcessorBL1)
add_subdirectory(ServiceProcessorBL2)

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
  NAMESPACE esperanto-bl::
  DESTINATION "${LIB_INSTALL_DIR}/cmake/EsperantoBootLoader")
