#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------
include(ExternalProject)
include(GoogleTest)

set(GTEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/gtest-install)

ExternalProject_Add(googletest
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
  GIT_REPOSITORY git@gitlab.esperanto.ai:software/gtest-mirror.git
  GIT_TAG release-1.8.1
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-src"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
  INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest"
  CMAKE_ARGS = "-DCMAKE_INSTALL_PREFIX=${GTEST_DIR}"
  )

include_directories(${GTEST_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/src/device-api/flatc-gen-code/)
include_directories(${PROJECT_BINARY_DIR}/src/device-api/flatcc-output/)
include_directories(${PROJECT_BINARY_DIR}/src/device-api/flatcc-build/include)

link_directories(${GTEST_DIR}/lib64)
link_directories(${PROJECT_BINARY_DIR}/src/device-api/flatcc-build/lib)

add_compile_options(-g3 -std=c++11 -O0)

add_executable(test-device-api-cxx test_device_api_cxx.cc)
add_dependencies(test-device-api-cxx googletest parse-device-api)
target_link_libraries(test-device-api-cxx
  flatbuffers
  libgtest_main.a
  libgtest.a
  pthread
  )
gtest_discover_tests(test-device-api-cxx TEST_PREFIX device-api-cxx:)


add_executable(test-device-api-c test_device_api_c.cc)
add_dependencies(test-device-api-c googletest parse-device-api)
target_link_libraries(test-device-api-c
  flatcc
  libgtest_main.a
  libgtest.a
  pthread
  )

gtest_discover_tests(test-device-api-c TEST_PREFIX device-api-c:)
