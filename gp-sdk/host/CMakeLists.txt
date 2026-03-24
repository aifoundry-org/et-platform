#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

project(gp-sdk-host VERSION 1.6.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

option(BUILD_DOCS "Enable/disable building documentation" OFF)
option(BUILD_TESTS "Build example launchers" ON)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

find_package(deviceApi REQUIRED)
find_package(deviceLayer REQUIRED)
find_package(runtime REQUIRED)
find_package(esperantoTrace REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3F REQUIRED fftw3f)

include(SanitizerSupport)

add_subdirectory(sdk)

if (BUILD_TESTS)
  add_subdirectory(tests)
endif()

## project documentation
if (BUILD_DOCS)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../docs/host ${CMAKE_SOURCE_DIR}/../docs/host/build)
endif()

# project-specific directories to be added here
