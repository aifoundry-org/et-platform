#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

project(gp-sdk-device VERSION 1.6.0 LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_DOCS "Enable/disable building documentation" OFF)
option(BUILD_TESTS "Build GP-SDK test kernels" ON)
set(GP_SDK_TOOLS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tools" CACHE PATH "Path to GP-SDK tools")
set(GP_SDK_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/sdk/lib/linker.ld" CACHE PATH "Path to device linker script")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(STATUS "Configuring Device build for clang Compiler.. ")
  string(APPEND CMAKE_ASM_FLAGS " ${CMAKE_C_FLAGS}")
else()
  message(STATUS "Configuring Device build for gcc Compiler.. ")
endif()
 

if (NOT ADDRESS)
  message(FATAL_ERROR "ADDRESS parameter is missing. Please provide a value (e.g: -DADDRESS:STRING=0x8006335000")
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(et-common-libs REQUIRED COMPONENTS cm-umode erbium-umode erbium-soc1sim)
find_package(dnnLibrary)

include("${CMAKE_CURRENT_SOURCE_DIR}/riscv_helpers.cmake")

## project documentation
if (BUILD_DOCS)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../docs/device ${CMAKE_SOURCE_DIR}/../docs/device/build)
endif()

add_subdirectory(sdk)
if (BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Users of this CMakeLists.txt can provide an additional subdirectory by passing build arguments,
# as opposed to modifying this CMakeLists.txt. This CMake configuration mechanism is intended for
# consumers of the GP-SDK to build custom kernels leveraging the same build recipe from the
# internal GP-SDK tests. The accepted parameters are:
#
# - CUSTOM_KERNELS_SRC_DIR: directory where the sources and the corresponding CMakeLists.txt
#   are located. It can be located out of this tree.
#
# - CUSTOM_KERNELS_BIN_DIR: directory where the binary files will be stored. If the directory
#   does not exist it gets created.
#
# If one such pair of arguments is provided it is handled at this stage.

if (CUSTOM_KERNELS_SRC_DIR AND CUSTOM_KERNELS_BIN_DIR)
    add_subdirectory(${CUSTOM_KERNELS_SRC_DIR} ${CUSTOM_KERNELS_BIN_DIR})
endif()

# Users can also hack their way by just editing this file and have their own project added as a
# subdirectory here without any extra arguments being passed. That build method is suggested in
# the tutorial from the GP-SDK user guide but has also been labeled as not ideal for production by
# some users of the GP-SDK.
#
# add_subdirectory(tutorial_example)
