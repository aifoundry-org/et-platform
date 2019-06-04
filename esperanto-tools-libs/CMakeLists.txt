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

project(EsperantoRuntime VERSION 0.1.0 LANGUAGES CXX C)

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(HOST_SW "Enable building for the host-sw repo")

option(BUILD_SHARED_LIBS "Build shared libs." ON)
option(ETSOC "Build for the ETSOC")
option(ENABLE_TESTS "Enable tests" ON)
# To be overwritten in cases where we want to also "release" the external tools
# used by the runtime
set(RT_EXTERNAL_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external-install"
  CACHE PATH "Instalation path of external dependencies")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "HOST_SW=" ${HOST_SW})
message(STATUS "ETSOC=" ${ETSOC})
message(STATUS "ENABLE_TESTS=" ${ENABLE_TESTS})

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif()

set(external_project_dependencies "" CACHE STRING "External project list")

# Add rules for auto-generated headers.
add_subdirectory(include)
add_subdirectory(docs)
add_subdirectory(external)
add_subdirectory(src)
if (ENABLE_TESTS)
  add_subdirectory(tests)
  add_subdirectory(tools)
endif(ENABLE_TESTS)
