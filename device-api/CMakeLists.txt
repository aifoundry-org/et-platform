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

project(DeviceAPI LANGUAGES C CXX)

#set(CMAKE_VERBOSE_MAKEFILE ON)

enable_testing()

include(CMake/BuildFlatBuffers.cmake)

add_subdirectory(tools/flatbuffers
                 ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
                 EXCLUDE_FROM_ALL)

############################ Documentation #####################################

option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

add_subdirectory(src)
